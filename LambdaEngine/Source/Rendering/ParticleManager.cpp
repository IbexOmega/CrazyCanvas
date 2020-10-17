#include "Rendering/ParticleManager.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Buffer.h"

#include "Rendering/RenderGraph.h"
#include "Rendering/RenderAPI.h"

#include "Math/Random.h"

namespace LambdaEngine
{
	void ParticleManager::Init(uint32 maxParticleCapacity)
	{
		m_MaxParticleCount = maxParticleCapacity;
		m_Particles.Reserve(m_MaxParticleCount);

		constexpr uint32 chunkReservationSize = 10;
		m_FreeParticleChunks.Reserve(chunkReservationSize);

		// Initilize Default Particle Texture
		m_DefaultAtlasTextureGUID = ResourceManager::LoadTextureFromFile("Particles/ParticleAtlas.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		constexpr uint32 DEFAULT_ATLAS_TILE_SIZE = 64U;
		CreateAtlasTextureInstance(m_DefaultAtlasTextureGUID, DEFAULT_ATLAS_TILE_SIZE);

		// Create one particle chunk spanning the whole particle array
		ParticleChunk chunk = {};
		chunk.Offset = 0;
		chunk.Size = m_MaxParticleCount;

		m_FreeParticleChunks.PushBack(chunk);

		m_DirtyIndexBuffer = true;
		m_DirtyVertexBuffer = true;
	}

	void ParticleManager::Release()
	{
		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[b];
			for (DeviceChild* pResource : resourcesToRemove)
			{
				SAFERELEASE(pResource);
			}

			resourcesToRemove.Clear();

			SAFERELEASE(m_ppVertexStagingBuffer[b]);
			SAFERELEASE(m_ppParticleStagingBuffer[b]);
			SAFERELEASE(m_ppIndexStagingBuffer[b]);
			SAFERELEASE(m_ppIndirectStagingBuffer[b]);
			SAFERELEASE(m_ppAtlasDataStagingBuffer[b]);
		}

		SAFERELEASE(m_pIndirectBuffer);
		SAFERELEASE(m_pVertexBuffer);
		SAFERELEASE(m_pIndexBuffer);
		SAFERELEASE(m_pParticleBuffer);
		SAFERELEASE(m_pAtlasDataBuffer);

		if (m_Sampler)
			SAFERELEASE(m_Sampler);
	}

	void ParticleManager::Tick(Timestamp deltaTime, uint32 modFrameIndex)
	{
		m_ModFrameIndex = modFrameIndex;

		for (auto activeEmitterIt = m_ActiveEmitters.begin();  activeEmitterIt != m_ActiveEmitters.end();)
		{
			if (activeEmitterIt->second.OneTime)
			{
				float& elapTime = activeEmitterIt->second.ElapTime;
				elapTime += deltaTime.AsSeconds();

				if (elapTime >= activeEmitterIt->second.LifeTime)
				{
					activeEmitterIt = m_ActiveEmitters.erase(activeEmitterIt);
					continue;
				}
			}
			activeEmitterIt++;
		}
	}

	void ParticleManager::OnEmitterEntityAdded(Entity entity)
	{
		ECSCore* ecsCore = ECSCore::GetInstance();
		PositionComponent positionComp = ecsCore->GetComponent<PositionComponent>(entity);
		RotationComponent rotationComp = ecsCore->GetComponent<RotationComponent>(entity);
		ParticleEmitterComponent emitterComp = ecsCore->GetComponent<ParticleEmitterComponent>(entity);

		ParticleEmitterInstance instance = {};
		instance.Position			= positionComp.Position;
		instance.Rotation			= rotationComp.Quaternion;
		instance.ParticleChunk.Size = emitterComp.ParticleCount;
		instance.Angle				= emitterComp.Angle;
		instance.Velocity			= emitterComp.Velocity;
		instance.Acceleration		= emitterComp.Acceleration;
		instance.LifeTime			= emitterComp.LifeTime;
		instance.ParticleRadius		= emitterComp.ParticleRadius;

		GUID_Lambda atlasGUID = emitterComp.AtlasGUID;
		if (atlasGUID == GUID_NONE)
			atlasGUID = m_DefaultAtlasTextureGUID;
		if (!m_AtlasResources.contains(atlasGUID))
		{
			CreateAtlasTextureInstance(atlasGUID, emitterComp.AtlasTileSize);
		}

		instance.AnimationCount = emitterComp.AnimationCount;
		instance.AtlasIndex		= m_AtlasResources[emitterComp.AtlasGUID].AtlasIndex;
		instance.TileIndex		= emitterComp.TextureIndex;

		if (emitterComp.EmitterShape == EEmitterShape::CONE)
		{
			if (!CreateConeParticleEmitter(instance))
			{
				LOG_WARNING("[ParticleManager]: Failed to allocate Emitter Particles. Max particle capacity of %d exceeded!", m_Particles.GetSize());
				return;
			}
		}
	
		if (emitterComp.Active)
		{
			instance.IndirectDataIndex = m_IndirectData.GetSize();
			m_IndirectDataToEntity[instance.IndirectDataIndex] = entity;

			IndirectData indirectData;
			indirectData.FirstInstance	= instance.ParticleChunk.Offset;
			indirectData.InstanceCount	= instance.ParticleChunk.Size;
			indirectData.FirstIndex		= 0;
			indirectData.VertexOffset	= 0;
			indirectData.IndexCount		= 6;
			m_IndirectData.PushBack(indirectData);

			m_ActiveEmitters[entity] = instance;

			m_DirtyIndirectBuffer = true;
			m_DirtyParticleBuffer = true;
		}
		else
		{
			m_SleepingEmitters[entity] = instance;
		}
	}

	void ParticleManager::OnEmitterEntityRemoved(Entity entity)
	{
		ParticleChunk newFreeChunk;
		if (m_ActiveEmitters.find(entity) != m_ActiveEmitters.end())
		{
			// Remove emitter
			auto& emitter = m_ActiveEmitters[entity];

			// Remove indirect draw call
			uint32 removeIndex = emitter.IndirectDataIndex;
			if (removeIndex < m_IndirectData.GetSize())
			{
				uint32 lastIndex = m_IndirectData.GetSize() - 1U;
				Entity lastEmitter = m_IndirectDataToEntity[lastIndex];

				m_IndirectData[removeIndex] = m_IndirectData[lastIndex];
				m_ActiveEmitters[lastEmitter].IndirectDataIndex = removeIndex;
				m_IndirectDataToEntity[removeIndex] = lastEmitter;

				m_IndirectDataToEntity.erase(lastIndex);
				m_IndirectData.PopBack();
				m_DirtyIndirectBuffer = true;
			}
			else
			{
				LOG_WARNING("[ParticleManager]: Trying to remove non-exsisting indirectDrawData");
			}

			newFreeChunk = emitter.ParticleChunk;
			m_ActiveEmitters.erase(entity);

		}
		else if (m_SleepingEmitters.find(entity) != m_SleepingEmitters.end())
		{
			newFreeChunk = m_SleepingEmitters[entity].ParticleChunk;
			m_SleepingEmitters.erase(entity);
		}
		else
		{
			LOG_ERROR("[ParticleManager]: Trying to remove non-exsisting emitter");
			return;
		}

		FreeParticleChunk(newFreeChunk);
	}

	bool ParticleManager::CreateAtlasTextureInstance(GUID_Lambda atlasGUID, uint32 tileSize)
	{
		// Add atlas texture
		TextureView* textureView = ResourceManager::GetTextureView(atlasGUID);
		m_AtlasTextureViews.PushBack(textureView);

		if (!m_Sampler)
		{
			SamplerDesc samplerDesc = {};
			samplerDesc.DebugName = "Atlas Sampler";
			samplerDesc.MinFilter = EFilterType::FILTER_TYPE_LINEAR;
			samplerDesc.MagFilter = EFilterType::FILTER_TYPE_LINEAR;
			samplerDesc.MipmapMode = EMipmapMode::MIPMAP_MODE_NEAREST;
			samplerDesc.AddressModeU = ESamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerDesc.AddressModeV = ESamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerDesc.AddressModeW = ESamplerAddressMode::SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			m_Sampler = MakeSharedRef<Sampler>(RenderAPI::GetDevice()->CreateSampler(&samplerDesc));
		}
		m_AtlasSamplers.PushBack(m_Sampler.Get());

		uint32 width = textureView->GetDesc().pTexture->GetDesc().Width;
		uint32 height = textureView->GetDesc().pTexture->GetDesc().Height;

		// Add atlas information
		SAtlasInfo atlasInfo = {};
		atlasInfo.ColCount = height / tileSize;
		atlasInfo.RowCount = width / tileSize;
		atlasInfo.TileFactorX = (float)tileSize / (float)width;
		atlasInfo.TileFactorY = (float)tileSize / (float)width;
		atlasInfo.AtlasIndex = m_AtlasInfoData.GetSize();
		m_AtlasResources[atlasGUID] = atlasInfo;
		m_AtlasInfoData.PushBack(atlasInfo);
		m_DirtyAtlasDataBuffer = true;

		return true;
	}

	bool ParticleManager::CreateConeParticleEmitter(ParticleEmitterInstance& emitterInstance)
	{
		// TODO: Handle override max capacity particle request
		if (m_FreeParticleChunks.IsEmpty())
			return false;

		// Assign fitting chunk to emitter
		bool foundChunk = false;
		for (uint32 i = 0; i < m_FreeParticleChunks.GetSize(); i++)
		{
			ParticleChunk& freeChunk = m_FreeParticleChunks[i];
			ParticleChunk& emitterChunk = emitterInstance.ParticleChunk;

			if (emitterInstance.ParticleChunk.Size <= freeChunk.Size)
			{
				emitterChunk.Offset = freeChunk.Offset;

				uint32 diff = freeChunk.Size - emitterChunk.Size;
				if (diff == 0)
				{
					m_FreeParticleChunks.Erase(m_FreeParticleChunks.Begin() + i);
				}
				else
				{
					freeChunk.Offset += emitterChunk.Size;
					freeChunk.Size -= emitterChunk.Size;
				}

				foundChunk = true;
			}
		}

		// TODO: Handle override max capacity particle request
		if (!foundChunk)
			return false;

		bool allocateParticles = false;
		if (emitterInstance.ParticleChunk.Offset + emitterInstance.ParticleChunk.Size > m_Particles.GetSize())
		{
			allocateParticles = true;
		}

		const glm::vec3 forward = GetForward(emitterInstance.Rotation);
		const glm::vec3 up = GetUp(emitterInstance.Rotation);
		const glm::vec3 right = GetRight(emitterInstance.Rotation);
		const float		halfAngle = emitterInstance.Angle * 0.5f;

		uint32 particlesToAdd = emitterInstance.ParticleChunk.Size;
		const uint32 particleOffset = emitterInstance.ParticleChunk.Offset;
		for (uint32 i = 0; i < particlesToAdd; i++)
		{
			SParticle particle;

			glm::vec3 direction = forward;

			direction = glm::rotate(direction, Random::Float32(-glm::radians(halfAngle), glm::radians(halfAngle)), up);
			direction = glm::rotate(direction, Random::Float32(-glm::radians(halfAngle), glm::radians(halfAngle)), right);

			direction = glm::normalize(direction);

			particle.Transform = glm::translate(emitterInstance.Position);
			particle.StartPosition = emitterInstance.Position;
			particle.Color = glm::vec4(1.0f);
			particle.Velocity = direction * emitterInstance.Velocity;
			particle.StartVelocity = particle.Velocity;
			particle.Acceleration = direction * emitterInstance.Acceleration;
			particle.CurrentLife = emitterInstance.LifeTime;
			particle.LifeTime = emitterInstance.LifeTime;
			particle.Radius = emitterInstance.ParticleRadius;
			particle.AtlasIndex = emitterInstance.AtlasIndex;
			particle.TileIndex	= emitterInstance.TileIndex;
			particle.AnimationCount = emitterInstance.AnimationCount;

			if (allocateParticles)
			{
				m_Particles.PushBack(particle);
			}
			else
			{
				m_Particles[particleOffset + i] = particle;
			}
		}

		return true;
	}
	
	bool ParticleManager::CopyDataToBuffer(CommandList* pCommandList, void* data, uint64 size, Buffer** pStagingBuffers, Buffer** pBuffer, FBufferFlags flags, const String& name)
	{
		Buffer* pStagingBuffer = pStagingBuffers[m_ModFrameIndex];

		if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < size)
		{
			if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name + " Staging Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes = size;

			pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			pStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
		}

		void* pMapped = pStagingBuffer->Map();
		memcpy(pMapped, data, size);
		pStagingBuffer->Unmap();

		if ((*pBuffer) == nullptr || (*pBuffer)->GetDesc().SizeInBytes < size)
		{
			if ((*pBuffer) != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack((*pBuffer));

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = name;
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | flags;
			bufferDesc.SizeInBytes = size;

			(*pBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
		}
		else
		{
			return false; // Only update resource when buffer is recreated
		}

		pCommandList->CopyBuffer(pStagingBuffer, 0, (*pBuffer), 0, size);
		return true;
	}

	bool ParticleManager::FreeParticleChunk(ParticleChunk chunk)
	{
		// Find if freed particle chunk can be merged with existing one
		bool inserted = false;
	
		inserted = MergeParticleChunk(chunk);

		// If merge was not possible insert at proper offset location
		for (auto chunkIt = m_FreeParticleChunks.begin(); chunkIt != m_FreeParticleChunks.end() && !inserted; chunkIt++)
		{
			if (chunk.Offset > chunkIt->Offset)
			{
				m_FreeParticleChunks.Insert(chunkIt, chunk);
				inserted = true;
			}
		}

		// If free chunk has not been inserted yet beginning should be the proper insertion
		if (!inserted) {
			m_FreeParticleChunks.Insert(m_FreeParticleChunks.begin(), chunk);
			inserted = true;
		}

		return inserted;
	}

	bool ParticleManager::MergeParticleChunk(const ParticleChunk& chunk)
	{
		bool inserted = false;
		for (auto chunkIt = m_FreeParticleChunks.begin(); chunkIt != m_FreeParticleChunks.end() && !inserted; chunkIt++)
		{
			if (chunk.Offset + chunk.Size == chunkIt->Offset)
			{
				chunkIt->Offset -= chunk.Size;
				chunkIt->Size += chunk.Size;
				// If merge successful remove from chunk
				if (MergeParticleChunk(*chunkIt))
				{
					m_FreeParticleChunks.Erase(chunkIt);
				}
				inserted = true;
			}
			else if (chunkIt->Offset + chunkIt->Size == chunk.Offset)
			{
				chunkIt->Size += chunk.Size;
				// If merge successful remove from chunk
				if (MergeParticleChunk(*chunkIt))
				{
					m_FreeParticleChunks.Erase(chunkIt);
				}
				inserted = true;
			}
		}

		return inserted;
	}

	void ParticleManager::CleanBuffers()
	{
		TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[m_ModFrameIndex];
		for (DeviceChild* pResource : resourcesToRemove)
		{
			SAFERELEASE(pResource);
		}

		resourcesToRemove.Clear();
	}

	bool ParticleManager::UpdateBuffers(CommandList* pCommandList)
	{
		// Update Instance Buffer
		if (m_DirtyIndirectBuffer)
		{
			uint32 requiredBufferSize = m_IndirectData.GetSize() * sizeof(IndirectData);
			m_DirtyIndirectBuffer = CopyDataToBuffer(
				pCommandList, 
				m_IndirectData.GetData(), 
				requiredBufferSize, 
				m_ppIndirectStagingBuffer, 
				&m_pIndirectBuffer,
				FBufferFlag::BUFFER_FLAG_INDIRECT_BUFFER, 
				"Particle Indirect");
		}


		// Update Vertex Buffer
		if (m_DirtyVertexBuffer)
		{
			const glm::vec4 vertices[4] =
			{
				glm::vec4(-1.0, -1.0, 0.0, 1.0f),
				glm::vec4(-1.0, 1.0, 0.0, 1.0f),
				glm::vec4(1.0, -1.0, 0.0, 1.0f),
				glm::vec4(1.0, 1.0, 0.0, 1.0f),
			};

			uint32 requiredBufferSize = 4 * sizeof(glm::vec4);
			m_DirtyVertexBuffer = CopyDataToBuffer(
				pCommandList,
				(void*)vertices,
				requiredBufferSize,
				m_ppVertexStagingBuffer,
				&m_pVertexBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Particle Vertex");
		}
	

		// Update Index Buffer
		if (m_DirtyIndexBuffer)
		{
			const uint32 indices[6] =
			{
				2,1,0,
				2,3,1
			};

			uint32 requiredBufferSize = sizeof(uint32) * 6;
			m_DirtyIndexBuffer = CopyDataToBuffer(
				pCommandList,
				(void*)indices,
				requiredBufferSize,
				m_ppIndexStagingBuffer,
				&m_pIndexBuffer,
				FBufferFlag::BUFFER_FLAG_INDEX_BUFFER,
				"Particle Index");
		}
		
		// Update Particle Instance Buffer
		if (m_DirtyParticleBuffer)
		{
			uint32 requiredBufferSize = m_Particles.GetSize() * sizeof(SParticle);
			m_DirtyParticleBuffer = CopyDataToBuffer(
				pCommandList,
				m_Particles.GetData(),
				requiredBufferSize,
				m_ppParticleStagingBuffer,
				&m_pParticleBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Particle Instance");
		}

		// Update Atlas data Buffer
		if (m_DirtyAtlasDataBuffer)
		{
			uint32 requiredBufferSize = m_AtlasInfoData.GetSize() * sizeof(SAtlasInfo);
			m_DirtyAtlasDataBuffer = CopyDataToBuffer(
				pCommandList,
				m_AtlasInfoData.GetData(),
				requiredBufferSize,
				m_ppAtlasDataStagingBuffer,
				&m_pAtlasDataBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Atlas data");
		}

		return true;
	}

	bool ParticleManager::UpdateResources(RenderGraph* pRendergraph)
	{
		if (m_DirtyIndirectBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_INDIRECT_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pIndirectBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);

			m_DirtyIndirectBuffer = false;
		}

		if (m_DirtyVertexBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_VERTEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pVertexBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);

			m_DirtyVertexBuffer = false;
		}

		if (m_DirtyIndexBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_INDEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pIndexBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);

			m_DirtyIndexBuffer = false;
		}

		if (m_DirtyParticleBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_INSTANCE_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pParticleBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);

			m_DirtyParticleBuffer = false;
		}

		if (m_DirtyAtlasDataBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_ATLAS_INFO_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pAtlasDataBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);

			m_DirtyAtlasDataBuffer = false;
		}

		return false;
	}
}