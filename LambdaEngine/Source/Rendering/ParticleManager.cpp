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
		}

		SAFERELEASE(m_pIndirectBuffer);
		SAFERELEASE(m_pVertexBuffer);
		SAFERELEASE(m_pIndexBuffer);
		SAFERELEASE(m_pParticleBuffer);
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
		instance.Position = positionComp.Position;
		instance.Rotation = rotationComp.Quaternion;
		instance.ParticleChunk.Size = emitterComp.ParticleCount;
		instance.Angle = emitterComp.Angle;
		instance.Velocity = emitterComp.Velocity;
		instance.Acceleration = emitterComp.Acceleration;
		instance.LifeTime = emitterComp.LifeTime;
		instance.ParticleRadius = emitterComp.ParticleRadius;

		if (!m_AtlasResources.contains(emitterComp.AtlasGUID))
		{
			Texture* texture = ResourceManager::GetTexture(emitterComp.AtlasGUID);
			uint32 width = texture->GetDesc().Width;
			uint32 height = texture->GetDesc().Height;
			uint32 tileSize = emitterComp.AtlasTileSize;

			AtlasInfo atlasInfo = {};
			atlasInfo.ColCount = height / tileSize;
			atlasInfo.RowCount = width / tileSize;
			atlasInfo.TileFactorX = tileSize / width;
			atlasInfo.TileFactorY = tileSize / width;
			m_AtlasResources[emitterComp.AtlasGUID] = atlasInfo;
		}

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
			uint32 indirectIndex = emitter.IndirectDataIndex;
			if (indirectIndex < m_IndirectData.GetSize())
			{
				m_IndirectData[indirectIndex] = m_IndirectData.GetBack();
				m_IndirectData.PopBack();
				m_DirtyIndirectBuffer = true;
			}
			else
			{
				LOG_WARNING("[ParticleManager]: Trying to remove non-exsisting indirectDrawData");
			}

			m_ActiveEmitters.erase(entity);

			newFreeChunk = emitter.ParticleChunk;
		}
		else
		{
			LOG_ERROR("[ParticleManager]: Trying to remove non-exsisting emitter");
			return;
		}

		if (m_SleepingEmitters.find(entity) != m_SleepingEmitters.end())
		{
			newFreeChunk = m_SleepingEmitters[entity].ParticleChunk;
			m_SleepingEmitters.erase(entity);


		}
		else
		{
			LOG_ERROR("[ParticleManager]: Trying to remove non-exsisting emitter");
			return;
		}

		// Update particle buffer with new inactive particles
		m_DirtyParticleBuffer = true;

		// Free particles for new emitters
		for (auto chunkIt = m_FreeParticleChunks.begin(); chunkIt != m_FreeParticleChunks.end();)
		{
			if (newFreeChunk.Offset > chunkIt->Offset)
			{
				if (chunkIt->Offset + chunkIt->Size == newFreeChunk.Offset)
				{
					chunkIt->Size += newFreeChunk.Size;
				}
				else
				{
					m_FreeParticleChunks.Insert(chunkIt, newFreeChunk);
				}
			}
		}

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

		emitterInstance.ParticleChunk.Offset = m_Particles.GetSize();

		const glm::vec3 forward = GetForward(emitterInstance.Rotation);
		const glm::vec3 up = GetUp(emitterInstance.Rotation);
		const glm::vec3 right = GetRight(emitterInstance.Rotation);
		const float		halfAngle = emitterInstance.Angle * 0.5f;

		for (uint32 i = 0; i < emitterInstance.ParticleChunk.Size; i++)
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

			m_Particles.PushBack(particle);
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
			CopyDataToBuffer(
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
			CopyDataToBuffer(
				pCommandList,
				(void*)vertices,
				requiredBufferSize,
				m_ppVertexStagingBuffer,
				&m_pVertexBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Particle Vertex");
		}
		else
		{
			m_DirtyVertexBuffer = false; // Only update resource when buffer is recreated
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
			CopyDataToBuffer(
				pCommandList,
				(void*)indices,
				requiredBufferSize,
				m_ppIndexStagingBuffer,
				&m_pIndexBuffer,
				FBufferFlag::BUFFER_FLAG_INDEX_BUFFER,
				"Particle Index");
		}
		else
		{
			m_DirtyIndexBuffer = false; // Only update resource when buffer is recreated
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
		/*if (m_DirtyAtlasDataBuffer)
		{
			uint32 requiredBufferSize = m_Particles.GetSize() * sizeof(SParticle);
			m_DirtyAtlasDataBuffer = CopyDataToBuffer(
				pCommandList,
				m_Particles.GetData(),
				requiredBufferSize,
				m_ppAtlasDataStagingBuffer,
				&m_pAtlasDataBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Atlas data");
		}*/

		return true;
	}

	bool ParticleManager::UpdateResources(RenderGraph* pRendergraph)
	{
		if (m_DirtyIndirectBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = "INDIRECT_PARTICLES";
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


		return false;
	}
}