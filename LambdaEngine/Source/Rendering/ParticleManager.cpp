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
			SAFERELEASE(m_ppEmitterStagingBuffer[b]);
			SAFERELEASE(m_ppTransformStagingBuffer[b]);
			SAFERELEASE(m_ppIndirectStagingBuffer[b]);
			SAFERELEASE(m_ppAtlasDataStagingBuffer[b]);
		}

		SAFERELEASE(m_pIndirectBuffer);
		SAFERELEASE(m_pVertexBuffer);
		SAFERELEASE(m_pIndexBuffer);
		SAFERELEASE(m_pParticleBuffer);
		SAFERELEASE(m_pEmitterBuffer);
		SAFERELEASE(m_pTransformBuffer);
		SAFERELEASE(m_pAtlasDataBuffer);

		if (m_Sampler)
			SAFERELEASE(m_Sampler);
	}

	void ParticleManager::Tick(Timestamp deltaTime, uint32 modFrameIndex)
	{
		m_ModFrameIndex = modFrameIndex;

		constexpr float EPSILON = 0.01f;

		// Set active false if timer has elapsed
		ComponentArray<ParticleEmitterComponent>* pEmitterComponents = ECSCore::GetInstance()->GetComponentArray<ParticleEmitterComponent>();

		for (auto activeEmitterIt = m_ActiveEmitters.begin();  activeEmitterIt != m_ActiveEmitters.end();)
		{
			if (pEmitterComponents->HasComponent(activeEmitterIt->first))
			{
				const auto& constEmitter = pEmitterComponents->GetConstData(activeEmitterIt->first);
				if (activeEmitterIt->second.OneTime && constEmitter.Active)
				{
					float& elapTime = activeEmitterIt->second.ElapTime;
					elapTime += deltaTime.AsSeconds();

					if (elapTime >= activeEmitterIt->second.LifeTime - EPSILON)
					{
						// Set emitter component to inactive so this dose not trigger again and again
						auto& Emitter = pEmitterComponents->GetData(activeEmitterIt->first);
						Emitter.Active = false;
						elapTime = 0.f;

						DeactivateEmitterEntity(activeEmitterIt->second);
						m_SleepingEmitters[activeEmitterIt->first] = activeEmitterIt->second;
						activeEmitterIt = m_ActiveEmitters.erase(activeEmitterIt);
						continue;
					}
				}
			}
			activeEmitterIt++;
		}
	}

	void ParticleManager::UpdateParticleEmitter(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp)
	{
		// If emitter is active either update its transform if its already active or move it from sleeping to active if not
		if (emitterComp.Active)
		{
			auto emitterElem = m_ActiveEmitters.find(entity);
			// Check if active
			if (emitterElem != m_ActiveEmitters.end())
			{
				ParticleEmitterInstance& emitter = emitterElem->second;
				uint32 dataIndex = emitter.DataIndex;

				// Update transform of emitter
				m_EmitterTransformData[dataIndex] = glm::translate(positionComp.Position) * glm::toMat4(rotationComp.Quaternion);
				m_DirtyTransformBuffer = true;

				return;
			}

			emitterElem = m_SleepingEmitters.find(entity);
			if (emitterElem != m_SleepingEmitters.end())
			{
				ActivateEmitterEntity(emitterElem->second, positionComp, rotationComp, emitterComp);
				
				// Map emitter data to entity
				m_DataToEntity[emitterElem->second.DataIndex] = entity;
				
				// Move emitter from active to sleeping
				m_ActiveEmitters[emitterElem->first] = emitterElem->second;
				m_SleepingEmitters.erase(emitterElem->first);
			}
		}
	}

	void ParticleManager::OnEmitterEntityAdded(Entity entity)
	{
		ECSCore* ecsCore = ECSCore::GetInstance();
		const PositionComponent& positionComp = ecsCore->GetComponent<PositionComponent>(entity);
		const RotationComponent& rotationComp = ecsCore->GetComponent<RotationComponent>(entity);
		const ParticleEmitterComponent& emitterComp = ecsCore->GetComponent<ParticleEmitterComponent>(entity);

		ParticleEmitterInstance instance = {};
		instance.Position			= positionComp.Position;
		instance.Rotation			= rotationComp.Quaternion;
		instance.ParticleChunk.Size = emitterComp.ParticleCount;
		instance.OneTime			= emitterComp.OneTime;
		instance.Angle				= emitterComp.Angle;
		instance.Velocity			= emitterComp.Velocity;
		instance.Acceleration		= emitterComp.Acceleration;
		instance.LifeTime			= emitterComp.LifeTime;
		instance.ParticleRadius		= emitterComp.ParticleRadius * 0.5f;
		instance.Explosive			= emitterComp.Explosive;
		instance.SpawnSpeed			= emitterComp.SpawnSpeed;
		instance.Color				= emitterComp.Color;

		GUID_Lambda atlasGUID = emitterComp.AtlasGUID;
		if (atlasGUID == GUID_NONE)
			atlasGUID = m_DefaultAtlasTextureGUID;

		if (!m_AtlasResources.contains(atlasGUID))
		{
			CreateAtlasTextureInstance(atlasGUID, emitterComp.AtlasTileSize);
		}

		instance.AnimationCount = emitterComp.AnimationCount;
		instance.AtlasIndex = m_AtlasResources[emitterComp.AtlasGUID].AtlasIndex;
		instance.TileIndex = emitterComp.TileIndex;
		instance.AnimationCount = emitterComp.AnimationCount;
		instance.FirstAnimationIndex = emitterComp.FirstAnimationIndex;

		// Set data index before creation of particles so each particle now which emitter they belong to
		instance.DataIndex = m_IndirectData.GetSize();

		if (!AllocateParticleChunk(instance.ParticleChunk))
		{
			LOG_WARNING("[ParticleManager]: Failed to allocate Emitter Particles. Max particle capacity of %d exceeded!", m_Particles.GetSize());
			return;
		}

		if (emitterComp.EmitterShape == EEmitterShape::CONE)
		{
			CreateConeParticleEmitter(instance);
		}
		else if (emitterComp.EmitterShape == EEmitterShape::TUBE)
		{
			CreateTubeParticleEmitter(instance);
		}


		if (emitterComp.Active)
		{
			m_DataToEntity[instance.DataIndex] = entity;

			// Create IndirectDrawData
			IndirectData indirectData;
			indirectData.FirstInstance	= instance.ParticleChunk.Offset;
			indirectData.InstanceCount	= instance.ParticleChunk.Size;
			indirectData.FirstIndex		= 0;
			indirectData.VertexOffset	= 0;
			indirectData.IndexCount		= 6;
			m_IndirectData.PushBack(indirectData);

			// Create EmitterData
			SEmitter emitterData = {};
			emitterData.Color					= emitterComp.Color;
			emitterData.LifeTime				= emitterComp.LifeTime;
			emitterData.Radius					= instance.ParticleRadius;
			emitterData.AtlasIndex				= instance.AtlasIndex;
			emitterData.AnimationCount			= instance.AnimationCount;
			emitterData.FirstAnimationIndex		= instance.FirstAnimationIndex;
			emitterData.Gravity					= emitterComp.Gravity;
			m_EmitterData.PushBack(emitterData);

			// Create Transform
			glm::mat4 emitterTransform = glm::translate(instance.Position) * glm::toMat4(instance.Rotation);
			m_EmitterTransformData.PushBack(emitterTransform);

			// Add particle chunk to dirty list
			m_DirtyParticleChunks.PushBack(instance.ParticleChunk);

			m_ActiveEmitters[entity] = instance;

			m_DirtyIndirectBuffer = true;
			m_DirtyEmitterBuffer = true;
			m_DirtyTransformBuffer = true;
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

			DeactivateEmitterEntity(emitter);

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
			samplerDesc.MinFilter = EFilterType::FILTER_TYPE_NEAREST;
			samplerDesc.MagFilter = EFilterType::FILTER_TYPE_NEAREST;
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
		bool allocateParticles = false;
		if (emitterInstance.ParticleChunk.Offset + emitterInstance.ParticleChunk.Size > m_Particles.GetSize())
		{
			allocateParticles = true;
		}

		const glm::vec3 forward = g_DefaultForward;
		const glm::vec3 up = g_DefaultUp;
		const glm::vec3 right = g_DefaultRight;
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

			particle.StartPosition = glm::vec3(0.f);
			particle.Transform = glm::identity<glm::mat4>();
			particle.Velocity = direction * emitterInstance.Velocity;
			particle.StartVelocity = particle.Velocity;
			particle.Radius = emitterInstance.ParticleRadius;
			particle.Acceleration = direction * emitterInstance.Acceleration;
			particle.StartAcceleration = particle.Acceleration;
			particle.TileIndex = emitterInstance.TileIndex;
			particle.EmitterIndex = emitterInstance.DataIndex;
			particle.WasCreated = true;

			particle.LifeTime = emitterInstance.LifeTime;
			particle.CurrentLife = particle.LifeTime + (1.f - emitterInstance.Explosive) * i * emitterInstance.SpawnSpeed;

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

	bool ParticleManager::CreateTubeParticleEmitter(ParticleEmitterInstance& emitterInstance)
	{
		bool allocateParticles = false;
		if (emitterInstance.ParticleChunk.Offset + emitterInstance.ParticleChunk.Size > m_Particles.GetSize())
		{
			allocateParticles = true;
		}

		const glm::vec3 direction = g_DefaultForward;

		uint32 particlesToAdd = emitterInstance.ParticleChunk.Size;
		const uint32 particleOffset = emitterInstance.ParticleChunk.Offset;
		for (uint32 i = 0; i < particlesToAdd; i++)
		{
			SParticle particle;
 
			particle.StartPosition = direction * (i * emitterInstance.ParticleRadius);
			particle.Transform = glm::translate(particle.StartPosition);
			particle.Velocity = glm::vec3(0.f);
			particle.StartVelocity = particle.Velocity;
			particle.Radius = emitterInstance.ParticleRadius;
			particle.Acceleration = glm::vec3(0.f);
			particle.StartAcceleration = particle.Acceleration;
			particle.TileIndex = emitterInstance.TileIndex;
			particle.EmitterIndex = emitterInstance.DataIndex;
			particle.WasCreated = true;

			particle.LifeTime = emitterInstance.LifeTime;
			particle.CurrentLife = particle.LifeTime + (1.f - emitterInstance.Explosive) * i * emitterInstance.SpawnSpeed;

			if (allocateParticles)
			{
				m_Particles.PushBack(particle);
			}
			else
			{
				m_Particles[particleOffset + i] = particle;
			}
		}


		return allocateParticles;
	}
	
	bool ParticleManager::CopyDataToBuffer(CommandList* pCommandList, void* data, uint64* pOffsets, uint64* pSize, uint64 regionCount, Buffer** pStagingBuffers, Buffer** pBuffer, FBufferFlags flags, const String& name)
	{
		Buffer* pStagingBuffer = pStagingBuffers[m_ModFrameIndex];
		Buffer* pPreviousBuffer = nullptr;
		bool needUpdate = true;

		if (regionCount > 0)
		{
			uint32 neededSize = pOffsets[0] + pSize[0];
			// Find largest offset + size to determine needed buffer size;
			if (regionCount > 1)
			{
				for (uint64 i = 1; i < regionCount; i++)
				{
					uint32 size = pOffsets[i] + pSize[i];
					if (neededSize < size)
					{
						neededSize = size;
					}
				}
			}

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < neededSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = name + " Staging Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes = neededSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				pStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();

			for (uint32 r = 0; r < regionCount; r++)
			{
				memcpy((char*)pMapped + pOffsets[r], ((char*)data) + pOffsets[r], pSize[r]);
			}

			pStagingBuffer->Unmap();

			if ((*pBuffer) == nullptr || (*pBuffer)->GetDesc().SizeInBytes < neededSize)
			{
				if ((*pBuffer) != nullptr)
				{
					pPreviousBuffer = (*pBuffer);
					m_ResourcesToRemove[m_ModFrameIndex].PushBack((*pBuffer));
				}
				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = name;
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_COPY_SRC | flags;
				bufferDesc.SizeInBytes = neededSize;

				(*pBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}
			else
			{
				needUpdate = false; // Only update resource when buffer is recreated
			}

			// Copy old data to new buffer
			if (pPreviousBuffer != nullptr)
				pCommandList->CopyBuffer(pPreviousBuffer, 0, (*pBuffer), 0, pPreviousBuffer->GetDesc().SizeInBytes);

			for (uint32 r = 0; r < regionCount; r++)
			{
				pCommandList->CopyBuffer(pStagingBuffer, pOffsets[r], (*pBuffer), pOffsets[r], pSize[r]);
			}
		}
		else
			needUpdate = false;

		return needUpdate;
	}

	bool ParticleManager::ActivateEmitterEntity(ParticleEmitterInstance& emitterInstance, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp)
	{
		const ParticleChunk& chunk = emitterInstance.ParticleChunk;

		if (chunk.Size > 0) 
		{
			const uint32 offset = chunk.Offset;

			ParticleEmitterInstance instance = {};
			instance.Position = positionComp.Position;
			instance.Rotation = rotationComp.Quaternion;
			instance.ParticleChunk.Size = emitterComp.ParticleCount;
			instance.ParticleChunk.Offset = chunk.Offset;
			instance.OneTime = emitterComp.OneTime;
			instance.Angle = emitterComp.Angle;
			instance.Velocity = emitterComp.Velocity;
			instance.Acceleration = emitterComp.Acceleration;
			instance.LifeTime = emitterComp.LifeTime;
			instance.ParticleRadius = emitterComp.ParticleRadius * 0.5f;
			instance.AtlasIndex = emitterComp.AtlasGUID;
			instance.AnimationCount = emitterComp.AnimationCount;
			instance.TileIndex = emitterComp.TileIndex;
			instance.FirstAnimationIndex = emitterComp.FirstAnimationIndex;
			instance.SpawnSpeed = emitterComp.SpawnSpeed;
			instance.DataIndex = m_IndirectData.GetSize();
			instance.Color = emitterComp.Color;
			emitterInstance = instance;

			if (emitterComp.EmitterShape == EEmitterShape::CONE)
			{
				CreateConeParticleEmitter(instance);
			}
			else if (emitterComp.EmitterShape == EEmitterShape::TUBE)
			{
				CreateTubeParticleEmitter(instance);
			}
			// Add particle chunk to dirty list
			m_DirtyParticleChunks.PushBack(instance.ParticleChunk);

			m_DirtyParticleBuffer = true;

			// Create IndirectDrawData
			IndirectData indirectData;
			indirectData.FirstInstance = chunk.Offset;
			indirectData.InstanceCount = chunk.Size;
			indirectData.FirstIndex = 0;
			indirectData.VertexOffset = 0;
			indirectData.IndexCount = 6;
			m_IndirectData.PushBack(indirectData);

			// Create EmitterData
			SEmitter emitterData = {};
			emitterData.Color = emitterInstance.Color;
			emitterData.LifeTime = emitterInstance.LifeTime;
			emitterData.Radius = emitterInstance.ParticleRadius;
			emitterData.AnimationCount = emitterInstance.AnimationCount;
			emitterData.FirstAnimationIndex = emitterInstance.FirstAnimationIndex;
			emitterData.Gravity = emitterComp.Gravity;
			// Fetch default texture if none is set
			GUID_Lambda atlasGUID = emitterComp.AtlasGUID;
			if (atlasGUID == GUID_NONE)
				atlasGUID = m_DefaultAtlasTextureGUID;
			emitterData.AtlasIndex = m_AtlasResources[emitterComp.AtlasGUID].AtlasIndex;

			m_EmitterData.PushBack(emitterData);

			// Create Transform
			glm::mat4 emitterTransform = glm::translate(positionComp.Position) * glm::toMat4(rotationComp.Quaternion);
			m_EmitterTransformData.PushBack(emitterTransform);

			m_DirtyTransformBuffer = true;
			m_DirtyEmitterBuffer = true;
			m_DirtyIndirectBuffer = true;
		}

		return true;
	}

	bool ParticleManager::DeactivateEmitterEntity(const ParticleEmitterInstance& emitterInstance)
	{
		// Remove indirect draw call
		uint32 removeIndex = emitterInstance.DataIndex;
		if (removeIndex < m_IndirectData.GetSize())
		{
			uint32 lastIndex = m_IndirectData.GetSize() - 1U;
			Entity lastEmitterEntity = m_DataToEntity[lastIndex];

			// Replace last emitter with removed one
			m_IndirectData[removeIndex] = m_IndirectData[lastIndex];
			m_EmitterData[removeIndex] = m_EmitterData[lastIndex];
			m_EmitterTransformData[removeIndex] = m_EmitterTransformData[lastIndex];

			// Update last emitter data index to new one
			auto& lastEmitter = m_ActiveEmitters[lastEmitterEntity];
			lastEmitter.DataIndex = removeIndex;

			// Update particles of last emitter
			const ParticleChunk& chunk = lastEmitter.ParticleChunk;

			if (chunk.Size > 0)
			{
				const uint32 offset = chunk.Offset;

				for (uint32 i = offset; i < chunk.Size; i++)
				{
					m_Particles[i].EmitterIndex = lastEmitter.DataIndex;
				}
			}

			// Remove data index of remove emitter
			m_DataToEntity[removeIndex] = lastEmitterEntity;

			// Remove copy of last emitter
			m_DataToEntity.erase(lastIndex);
			m_IndirectData.PopBack();
			m_EmitterData.PopBack();
			m_EmitterTransformData.PopBack();

			m_DirtyIndirectBuffer = true;
			m_DirtyEmitterBuffer = true;
			m_DirtyTransformBuffer = true;
			m_DirtyParticleBuffer = true;
		}
		else
		{
			LOG_WARNING("[ParticleManager]: Trying to remove non-exsisting indirectDrawData");
			return false;
		}

		return true;
	}

	bool ParticleManager::AllocateParticleChunk(ParticleChunk& chunk)
	{
		// TODO: Handle override max capacity particle request
		if (m_FreeParticleChunks.IsEmpty())
			return false;

		// Assign fitting chunk to emitter
		bool foundChunk = false;
		for (uint32 i = 0; i < m_FreeParticleChunks.GetSize(); i++)
		{
			ParticleChunk& freeChunk = m_FreeParticleChunks[i];
			ParticleChunk& emitterChunk = chunk;

			if (emitterChunk.Size <= freeChunk.Size)
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

#if DEBUG_PARTICLE
		LOG_INFO("[ParticleManager]: Allocated Chunk[offset: %d, size: %d]", chunk.Offset, chunk.Size);
#endif

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

#if DEBUG_PARTICLE
		LOG_INFO("[ParticleManager]: Freed Chunk: [offset: %d, size : %d]", chunk.Offset, chunk.Size);
		LOG_INFO("[ParticleManager]: Current Free Chunks:");
		for (size_t i = 0; i < m_FreeParticleChunks.GetSize(); i++)
		{
			LOG_INFO("\tFree chunk%d - [offset: %d, size : %d] ", i, m_FreeParticleChunks[i].Offset, m_FreeParticleChunks[i].Size);
		}
#endif


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
		CleanBuffers();

		// Update Instance Buffer
		if (m_DirtyIndirectBuffer)
		{
			uint64 offset = 0;
			uint64 requiredBufferSize = m_IndirectData.GetSize() * sizeof(IndirectData);
			m_DirtyIndirectBuffer = CopyDataToBuffer(
				pCommandList, 
				m_IndirectData.GetData(), 
				&offset,
				&requiredBufferSize,
				1U,
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

			uint64 offset = 0;
			uint64 requiredBufferSize = 4 * sizeof(glm::vec4);
			m_DirtyVertexBuffer = CopyDataToBuffer(
				pCommandList,
				(void*)vertices,
				&offset,
				&requiredBufferSize,
				1U,
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

			uint64 offset = 0;
			uint64 requiredBufferSize = sizeof(uint32) * 6;
			m_DirtyIndexBuffer = CopyDataToBuffer(
				pCommandList,
				(void*)indices,
				&offset,
				&requiredBufferSize,
				1U,
				m_ppIndexStagingBuffer,
				&m_pIndexBuffer,
				FBufferFlag::BUFFER_FLAG_INDEX_BUFFER,
				"Particle Index");
		}
		
		// Update Particle Instance Buffer
		if (m_DirtyParticleBuffer)
		{
			// Only update dirty particle chunks
			uint64 dirtyChunks = m_DirtyParticleChunks.GetSize();
			TArray<uint64> offset(dirtyChunks);
			TArray<uint64> requiredBufferSize(dirtyChunks);
			for (size_t i = 0; i < m_DirtyParticleChunks.GetSize(); i++)
			{
				offset[i] = m_DirtyParticleChunks[i].Offset * sizeof(SParticle);
				requiredBufferSize[i] = m_DirtyParticleChunks[i].Size * sizeof(SParticle);
			}
			m_DirtyParticleChunks.Clear();

			m_DirtyParticleBuffer = CopyDataToBuffer(
				pCommandList,
				m_Particles.GetData(),
				offset.GetData(),
				requiredBufferSize.GetData(),
				dirtyChunks,
				m_ppParticleStagingBuffer,
				&m_pParticleBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Particle Instances");
		}

		// Update Emitter Instance Buffer
		if (m_DirtyEmitterBuffer)
		{
			uint64 offset = 0;
			uint64 requiredBufferSize = m_EmitterData.GetSize() * sizeof(SEmitter);
			m_DirtyEmitterBuffer = CopyDataToBuffer(
				pCommandList,
				m_EmitterData.GetData(),
				&offset,
				&requiredBufferSize,
				1U,
				m_ppEmitterStagingBuffer,
				&m_pEmitterBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Emitter Instances");
		}

		// Update Emitter Transform Buffer
		if (m_DirtyTransformBuffer)
		{
			uint64 offset = 0;
			uint64 requiredBufferSize = m_EmitterTransformData.GetSize() * sizeof(glm::mat4);
			m_DirtyTransformBuffer = CopyDataToBuffer(
				pCommandList,
				m_EmitterTransformData.GetData(),
				&offset,
				&requiredBufferSize,
				1U,
				m_ppTransformStagingBuffer,
				&m_pTransformBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Emitter Transforms");
		}

		// Update Atlas data Buffer
		if (m_DirtyAtlasDataBuffer)
		{
			uint64 offset = 0;
			uint64 requiredBufferSize = m_AtlasInfoData.GetSize() * sizeof(SAtlasInfo);
			m_DirtyAtlasDataBuffer = CopyDataToBuffer(
				pCommandList,
				m_AtlasInfoData.GetData(),
				&offset,
				&requiredBufferSize,
				1U,
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
		}
		m_DirtyIndirectBuffer = false;

		if (m_DirtyVertexBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_VERTEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pVertexBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyVertexBuffer = false;

		if (m_DirtyIndexBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_INDEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pIndexBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyIndexBuffer = false;

		if (m_DirtyParticleBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_INSTANCE_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pParticleBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyParticleBuffer = false;

		if (m_DirtyEmitterBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_EMITTER_INSTANCE_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pEmitterBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyEmitterBuffer = false;

		if (m_DirtyTransformBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_EMITTER_TRANSFORM_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pTransformBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyTransformBuffer = false;

		if (m_DirtyAtlasDataBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_ATLAS_INFO_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pAtlasDataBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyAtlasDataBuffer = false;

		return false;
	}
}