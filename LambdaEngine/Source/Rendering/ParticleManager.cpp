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
	void ParticleManager::Init(uint32 maxParticleCapacity, ASBuilder* pASBuilder)
	{
		if (!m_Initialized)
		{
			m_MaxParticleCount = maxParticleCapacity;
			m_Particles.Reserve(m_MaxParticleCount);
			m_AliveIndices.Reserve(m_MaxParticleCount);
			m_ParticleIndexData.Reserve(m_MaxParticleCount);

			constexpr uint32 chunkReservationSize = 10;
			m_FreeParticleChunks.Reserve(chunkReservationSize);

			// Initilize Default Particle Texture
			m_DefaultAtlasTextureGUID = ResourceManager::LoadTextureFromFile("Particles/ParticleAtlas.png", EFormat::FORMAT_R8G8B8A8_UNORM, true, true);
			constexpr uint32 DEFAULT_ATLAS_TILE_SIZE = 64U;
			CreateAtlasTextureInstance(m_DefaultAtlasTextureGUID, DEFAULT_ATLAS_TILE_SIZE);

			// Create one particle chunk spanning the whole particle array
			ParticleChunk chunk = {};
			chunk.Offset = 0;
			chunk.Size = m_MaxParticleCount;

			m_FreeParticleChunks.PushBack(chunk);

			BufferDesc bufferDesc = {};
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			bufferDesc.SizeInBytes	= sizeof(uint32);

			{
				bufferDesc.DebugName = "Particle Indirect Dummy Buffer";
				m_pIndirectBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			{
				bufferDesc.DebugName = "Particle Transform Dummy Buffer";
				m_pTransformBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			{
				bufferDesc.DebugName = "Particle Emitter Dummy Buffer";
				m_pEmitterBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}
			
			{
				bufferDesc.DebugName = "Particle Index Data Dummy Buffer";
				m_pParticleIndexDataBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			{
				bufferDesc.DebugName = "Particle Dummy Buffer";
				m_pParticleBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			{
				bufferDesc.DebugName = "Particle Alive Buffer";
				m_pAliveBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			m_DirtyEmitterBuffer = true;
			m_DirtyIndirectBuffer = true;
			m_DirtyTransformBuffer = true;
			m_DirtyEmitterIndexBuffer = true;
			m_DirtyParticleBuffer = true;
			m_DirtyAliveBuffer = true;

			m_pASBuilder = pASBuilder;
			// Create Billboard vertices and indices
			const uint32 VERTEX_COUNT = 4;
			const uint32 INDEX_COUNT = 6;
			bufferDesc = {};
			bufferDesc.DebugName = "Particle Billboard Vertex Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
			bufferDesc.SizeInBytes = sizeof(glm::vec4) * VERTEX_COUNT;
			m_pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			m_DirtyVertexBuffer = true;

			bufferDesc = {};
			bufferDesc.DebugName = "Particle Billboard Index Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER;
			bufferDesc.SizeInBytes = sizeof(uint32) * INDEX_COUNT;
			m_pIndexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			m_DirtyIndexBuffer = true;

			m_pASBuilder->BuildTriBLAS(m_BLASIndex, 1U, m_pVertexBuffer, m_pIndexBuffer, VERTEX_COUNT, sizeof(glm::vec4), INDEX_COUNT, false);

			m_Initialized = true;
		}
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
			SAFERELEASE(m_ppParticleIndexDataStagingBuffer[b]);
			SAFERELEASE(m_ppAliveStagingBuffer[b]);
			SAFERELEASE(m_ppTransformStagingBuffer[b]);
			SAFERELEASE(m_ppIndirectStagingBuffer[b]);
			SAFERELEASE(m_ppAtlasDataStagingBuffer[b]);
		}

		SAFERELEASE(m_pIndirectBuffer);
		SAFERELEASE(m_pVertexBuffer);
		SAFERELEASE(m_pIndexBuffer);
		SAFERELEASE(m_pParticleBuffer);
		SAFERELEASE(m_pEmitterBuffer);
		SAFERELEASE(m_pParticleIndexDataBuffer);
		SAFERELEASE(m_pAliveBuffer);
		SAFERELEASE(m_pTransformBuffer);
		SAFERELEASE(m_pAtlasDataBuffer);

		if (m_Sampler)
			SAFERELEASE(m_Sampler);
	}

	void ParticleManager::Tick(Timestamp deltaTime, uint64 modFrameIndex)
	{
		m_ModFrameIndex = modFrameIndex;

		constexpr float EPSILON = 0.01f;

		TArray<EmitterID> emittersToDeactivate;
		for (uint32 i = 0; i < m_Emitters.GetSize();)
		{
			auto& emitterInstance = m_Emitters[i];
			if (emitterInstance.OneTime)
			{
				float& elapTime = emitterInstance.ElapTime;
				elapTime += float(deltaTime.AsSeconds());

				float longestLifeTime = emitterInstance.LifeTime + (1.f - emitterInstance.Explosive) * (emitterInstance.ParticleChunk.Size - 1U) * emitterInstance.SpawnDelay;
				if (elapTime >= longestLifeTime - EPSILON)
				{
					DeactivateEmitterInstance(i);
					continue;
				}
			}
			i++;
		}
	}

	void ParticleManager::UpdateParticleEmitter(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp)
	{
		// If emitter is active either update its transform if its already active or move it from sleeping to active if not
		if (emitterComp.Active)
		{
			// Check if active
			auto emitterEntity = m_RepeatEmitters.find(entity);
			if (emitterEntity != m_RepeatEmitters.end())
			{
				EmitterID id = m_EntityToEmitterID[entity];

				// Update transform of emitter
				m_EmitterTransformData[id] = glm::translate(positionComp.Position) * glm::toMat4(rotationComp.Quaternion);
				m_DirtyTransformBuffer = true;

				ParticleEmitterInstance& emitterInstance = m_Emitters[id];
				UpdateEmitterInstanceData(emitterInstance, positionComp, rotationComp, emitterComp);

				if (emitterComp.Dirty)
				{
					// Update Emitter data
					SEmitter emitterData = {};
					emitterData.Color = emitterInstance.Color;
					emitterData.LifeTime = emitterInstance.LifeTime;
					emitterData.Radius = emitterInstance.BeginRadius;
					emitterData.AnimationCount = emitterInstance.AnimationCount;
					emitterData.FirstAnimationIndex = emitterInstance.FirstAnimationIndex;
					emitterData.Gravity = emitterInstance.Gravity;
					emitterData.OneTime = emitterInstance.OneTime;

					m_EmitterData[id] = emitterData;
					m_DirtyEmitterBuffer = true;
				}

				return;
			}
			else
			{
				ParticleEmitterInstance newEmitterInstance;
				EmitterID emitterID = m_Emitters.GetSize();
				m_Emitters.PushBack(newEmitterInstance);
				
				if (ActivateEmitterInstance(emitterID, positionComp, rotationComp, emitterComp))
				{
					// If not onetime emitter the emitter needs to be tracked for updating
					if (!emitterComp.OneTime)
					{
						// Move emitter from active to sleeping
						m_RepeatEmitters.insert(entity);

						m_EntityToEmitterID[entity] = emitterID;
						m_EmitterIDToEntity[emitterID] = entity;
					}
				}
				else
				{
					m_Emitters.PopBack();
				}
			}
		}
	}

	void ParticleManager::OnEmitterEntityRemoved(Entity entity)
	{
		// Deregister repeat emitter and change it to a onetime emitter
		if (m_RepeatEmitters.find(entity) != m_RepeatEmitters.end())
		{
			// Remove emitter
			EmitterID removeIndex = m_EntityToEmitterID[entity];

			// Removed tracking of repeat emitter since the emitter now will become a entity free emitter
			if (m_EmitterIDToEntity.find(removeIndex) != m_EmitterIDToEntity.end())
			{
				Entity removedEntity = m_EmitterIDToEntity[removeIndex];

				m_EntityToEmitterID.erase(removedEntity);
				m_EmitterIDToEntity.erase(removeIndex);

				m_RepeatEmitters.erase(removedEntity);
			}

			// Change emitter to a onetime emitter
			ParticleEmitterInstance& emitterInstance = m_Emitters[removeIndex];
			emitterInstance.OneTime = true;

			m_EmitterData[removeIndex].OneTime = true;

			m_DirtyEmitterBuffer = true;
			//DeactivateEmitterInstance(id);
		}
	}

	bool ParticleManager::CreateAtlasTextureInstance(GUID_Lambda atlasGUID, uint32 tileSize)
	{
		// Add atlas texture
		TextureView* pTextureView = ResourceManager::GetTextureView(atlasGUID);
		Texture* pTexture = ResourceManager::GetTexture(atlasGUID);
		m_AtlasTextureViews.PushBack(pTextureView);
		m_AtlasTextures.PushBack(pTexture);

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

		uint32 width = pTextureView->GetDesc().pTexture->GetDesc().Width;
		uint32 height = pTextureView->GetDesc().pTexture->GetDesc().Height;

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
		m_DirtyAtlasTexture = true;
		return true;
	}

	void ParticleManager::UpdateEmitterInstanceData(ParticleEmitterInstance& emitterInstance, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp)
	{
		emitterInstance.Position = positionComp.Position;
		emitterInstance.Rotation = rotationComp.Quaternion;

		emitterInstance.ParticleChunk.Size = emitterComp.ParticleCount;

		emitterInstance.OneTime = emitterComp.OneTime;
		emitterInstance.Explosive = emitterComp.Explosive;
		emitterInstance.SpawnDelay = emitterComp.SpawnDelay;
		emitterInstance.Angle = emitterComp.Angle;

		emitterInstance.Velocity = emitterComp.Velocity;
		emitterInstance.VelocityRandomness = emitterComp.VelocityRandomness;
		emitterInstance.Acceleration = emitterComp.Acceleration;
		emitterInstance.AccelerationRandomness = emitterComp.AccelerationRandomness;
		emitterInstance.Gravity = emitterComp.Gravity;

		emitterInstance.LifeTime = emitterComp.LifeTime;

		emitterInstance.RadiusRandomness = emitterComp.RadiusRandomness;
		emitterInstance.BeginRadius = emitterComp.BeginRadius * 0.5f;
		emitterInstance.EndRadius = emitterComp.EndRadius * 0.5f;

		emitterInstance.AtlasGUID = emitterComp.AtlasGUID;
		emitterInstance.AnimationCount = emitterComp.AnimationCount;
		emitterInstance.RandomStartIndex = emitterComp.RandomStartIndex;
		emitterInstance.FirstAnimationIndex = emitterComp.FirstAnimationIndex;

		emitterInstance.Color = emitterComp.Color;
		emitterInstance.FrictionFactor = emitterComp.FrictionFactor;

		emitterInstance.Color = emitterComp.Color;
	}

	void ParticleManager::ReplaceRemovedEmitterWithLast(uint32 removeIndex)
	{
		EmitterID lastIndex = m_Emitters.GetSize() - 1U;
		if (m_EmitterIDToEntity.find(lastIndex) != m_EmitterIDToEntity.end())
		{
			Entity lastEntity = m_EmitterIDToEntity[lastIndex];

			m_EmitterIDToEntity.erase(lastIndex);
			m_EntityToEmitterID[lastEntity] = removeIndex;
			m_EmitterIDToEntity[removeIndex] = lastEntity;
		}

		// Replace last emitter with removed emitter
		auto& lastEmitter = m_Emitters.GetBack();
		m_Emitters[removeIndex] = lastEmitter;
		m_Emitters.PopBack();

	}

	void ParticleManager::UpdateAliveParticles()
	{
		m_AliveIndices.Clear();
		for (const auto& indirect : m_IndirectData)
		{
			uint32 offset = indirect.FirstInstance;
			uint32 size = indirect.InstanceCount;
			for (uint32 particleIndex = offset; particleIndex < offset+size; particleIndex++)
			{
				m_AliveIndices.PushBack(particleIndex);
			}
		}
		m_DirtyAliveBuffer = true;
	}

	bool ParticleManager::CreateConeParticleEmitter(EmitterID emitterID)
	{
		auto& emitterInstance = m_Emitters[emitterID];

		const glm::vec3 forward = g_DefaultForward;
		const glm::vec3 up = g_DefaultUp;
		const glm::vec3 right = g_DefaultRight;
		const float		halfAngle = emitterInstance.Angle * 0.5f;

		// Add particle instances to TLAS
		uint32 particlesToAdd = emitterInstance.ParticleChunk.Size;
		const uint32 particleOffset = emitterInstance.ParticleChunk.Offset;
		const uint32 particleCount = m_Particles.GetSize();

		TArray<ASInstanceDesc> ASInstanceDescs;
		TArray<uint32> ASInstanceIndices;
		ASInstanceDescs.Reserve(particlesToAdd);
		ASInstanceIndices.Reserve(particlesToAdd);

		for (uint32 i = 0; i < particlesToAdd; i++)
		{
			SParticle particle;
			glm::vec3 direction = forward;

			direction = glm::rotate(direction, Random::Float32(-glm::radians(halfAngle), glm::radians(halfAngle)), up);
			direction = glm::rotate(direction, Random::Float32(-glm::radians(halfAngle), glm::radians(halfAngle)), right);

			direction = glm::normalize(direction);

			particle.StartPosition = glm::vec3(0.f);
			particle.Transform = glm::translate(emitterInstance.Position);
			particle.Velocity = direction * (emitterInstance.Velocity * (1.0f - emitterInstance.VelocityRandomness) + emitterInstance.Velocity * Random::Float32(0.f, emitterInstance.VelocityRandomness));
			particle.StartVelocity = particle.Velocity;
			particle.BeginRadius = (emitterInstance.BeginRadius * (1.0f - emitterInstance.RadiusRandomness) + emitterInstance.BeginRadius * Random::Float32(0.f, emitterInstance.RadiusRandomness));
			particle.EndRadius = emitterInstance.EndRadius;
			particle.Acceleration = direction * (emitterInstance.Acceleration * (1.0f - emitterInstance.AccelerationRandomness) + emitterInstance.Acceleration * Random::Float32(0.f, emitterInstance.AccelerationRandomness));
			particle.StartAcceleration = particle.Acceleration;
			particle.TileIndex = emitterInstance.RandomStartIndex ? (emitterInstance.FirstAnimationIndex + (i % emitterInstance.AnimationCount)) : emitterInstance.FirstAnimationIndex;
			particle.WasCreated = true;
			particle.FrictionFactor = emitterInstance.FrictionFactor;
			particle.ShouldStop = 0.f;

			particle.CurrentLife = emitterInstance.LifeTime + floor((1.f - emitterInstance.Explosive) * i) * emitterInstance.SpawnDelay;

			uint32 particleIndex = UINT32_MAX;
			if (particleOffset + i < particleCount)
			{
				particleIndex = particleOffset + i;
				m_Particles[particleOffset + i] = particle;
			}
			else
			{
				particleIndex = m_Particles.GetSize();
				m_Particles.PushBack(particle);
			}

			// Create ASInstanceDescs for RT
			ASInstanceDesc instanceDesc =
			{
				.BlasIndex = m_BLASIndex,
				.Transform = particle.Transform,
				.CustomIndex = particleIndex,
				.HitMask = 0x01,
				.Flags = RAY_TRACING_INSTANCE_FLAG_FRONT_CCW,
			};
			ASInstanceDescs.PushBack(instanceDesc);
		}

		// Add TLAS instances
		m_pASBuilder->AddInstances(ASInstanceDescs, ASInstanceIndices);
	
		for (uint32 i = 0; i < particlesToAdd; i++)
		{
			if (particleOffset + i < particleCount)
			{
				m_ParticleIndexData[particleOffset + i].EmitterIndex = emitterID;
				m_ParticleIndexData[particleOffset + i].ASInstanceIndirectIndex = ASInstanceIndices[i];
			}
			else
			{
				SParticleIndexData particleIndexData = {};
				particleIndexData.EmitterIndex = emitterID;
				particleIndexData.ASInstanceIndirectIndex = ASInstanceIndices[i];
				m_ParticleIndexData.PushBack(particleIndexData);
			}
		}

		return true;
	}

	bool ParticleManager::CreateTubeParticleEmitter(EmitterID emitterID)
	{
		auto& emitterInstance = m_Emitters[emitterID];

		const glm::vec3 direction = g_DefaultForward;

		uint32 particlesToAdd = emitterInstance.ParticleChunk.Size;
		const uint32 particleOffset = emitterInstance.ParticleChunk.Offset;
		const uint32 particleCount = m_Particles.GetSize();

		TArray<ASInstanceDesc> ASInstanceDescs;
		TArray<uint32> ASInstanceIndices;
		ASInstanceDescs.Reserve(particlesToAdd);
		ASInstanceIndices.Reserve(particlesToAdd);

		for (uint32 i = 0; i < particlesToAdd; i++)
		{
			SParticle particle;

			particle.StartPosition = direction * (i * emitterInstance.BeginRadius);
			particle.Transform = glm::translate(particle.StartPosition);
			particle.Velocity = glm::vec3(0.f);
			particle.StartVelocity = particle.Velocity;
			particle.BeginRadius = emitterInstance.BeginRadius;
			particle.EndRadius = emitterInstance.EndRadius;
			particle.Acceleration = glm::vec3(0.f);
			particle.StartAcceleration = particle.Acceleration;
			particle.TileIndex = emitterInstance.RandomStartIndex ? (emitterInstance.FirstAnimationIndex + (i % emitterInstance.AnimationCount)) : emitterInstance.FirstAnimationIndex;
			particle.WasCreated = true;
			particle.FrictionFactor = emitterInstance.FrictionFactor;
			particle.ShouldStop = 0.f;

			particle.CurrentLife = emitterInstance.LifeTime + (1.f - emitterInstance.Explosive) * i * emitterInstance.SpawnDelay;

			uint32 particleIndex = UINT32_MAX;
			if (particleOffset + i < particleCount)
			{
				particleIndex = particleOffset + i;
				m_Particles[particleOffset + i] = particle;
			}
			else
			{
				particleIndex = m_Particles.GetSize();
				m_Particles.PushBack(particle);
			}

			// Create ASInstanceDescs for RT
			ASInstanceDesc instanceDesc =
			{
				.BlasIndex = m_BLASIndex,
				.Transform = particle.Transform,
				.CustomIndex = particleIndex,
				.HitMask = 0xFF,
				.Flags = RAY_TRACING_INSTANCE_FLAG_FRONT_CCW,
			};
			ASInstanceDescs.PushBack(instanceDesc);
		}

		// Add TLAS instances
		m_pASBuilder->AddInstances(ASInstanceDescs, ASInstanceIndices);

		for (uint32 i = 0; i < particlesToAdd; i++)
		{
			if (particleOffset + i < particleCount)
			{
				m_ParticleIndexData[particleOffset + i].EmitterIndex = emitterID;
				m_ParticleIndexData[particleOffset + i].ASInstanceIndirectIndex = ASInstanceIndices[i];
			}
			else
			{
				SParticleIndexData particleIndexData = {};
				particleIndexData.EmitterIndex = emitterID;
				particleIndexData.ASInstanceIndirectIndex = ASInstanceIndices[i];
				m_ParticleIndexData.PushBack(particleIndexData);
			}
		}

		return true;
	}

	bool ParticleManager::CopyDataToBuffer(CommandList* pCommandList, void* data, uint32* pOffsets, uint32* pSize, uint32 regionCount, size_t elementSize, Buffer** ppStagingBuffers, Buffer** ppBuffer, FBufferFlags flags, const String& name)
	{
		Buffer* pStagingBuffer = ppStagingBuffers[m_ModFrameIndex];
		Buffer* pPreviousBuffer = nullptr;
		bool needUpdate = true;

		if (regionCount > 0)
		{
			size_t neededSize = pOffsets[0] + pSize[0];
			if (neededSize == 0)
				return false;

			// Find largest offset + size to determine needed buffer size;
			if (regionCount > 1)
			{
				for (uint32 i = 1; i < regionCount; i++)
				{
					uint32 size = pOffsets[i] + pSize[i];
					if (neededSize < size)
					{
						neededSize = size;
					}
				}
			}

			neededSize *= elementSize;

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < neededSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = name + " Staging Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes = neededSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				ppStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();

			for (uint32 r = 0; r < regionCount; r++)
			{
				memcpy((char*)pMapped + pOffsets[r] * elementSize, ((char*)data) + pOffsets[r] * elementSize, pSize[r] * elementSize);
			}

			pStagingBuffer->Unmap();

			if ((*ppBuffer) == nullptr || (*ppBuffer)->GetDesc().SizeInBytes < neededSize)
			{
				if ((*ppBuffer) != nullptr)
				{
					pPreviousBuffer = (*ppBuffer);
					m_ResourcesToRemove[m_ModFrameIndex].PushBack((*ppBuffer));
				}
				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = name;
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_COPY_SRC | flags;
				bufferDesc.SizeInBytes = neededSize;

				(*ppBuffer) = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}
			else
			{
				needUpdate = false; // Only update resource when buffer is recreated
			}

			// Copy old data to new buffer
			if (pPreviousBuffer != nullptr)
				pCommandList->CopyBuffer(pPreviousBuffer, 0, (*ppBuffer), 0, pPreviousBuffer->GetDesc().SizeInBytes);

			for (uint32 r = 0; r < regionCount; r++)
			{
				pCommandList->CopyBuffer(pStagingBuffer, pOffsets[r] * elementSize, (*ppBuffer), pOffsets[r] * elementSize, pSize[r] * elementSize);
			}
		}
		else
			needUpdate = false;

		return needUpdate;
	}

	bool ParticleManager::ActivateEmitterInstance(EmitterID emitterID, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp)
	{
		auto& emitterInstance = m_Emitters[emitterID];
		UpdateEmitterInstanceData(emitterInstance, positionComp, rotationComp, emitterComp);
		
		if (emitterInstance.ParticleChunk.Size > 0)
		{
			if (!AllocateParticleChunk(emitterInstance.ParticleChunk))
			{
				LOG_ERROR("[ParticleManager]: Failed to allocate Emitter Particles. Max particle capacity of %u exceeded!", m_Particles.GetSize());
				return false;
			}

			if (emitterComp.EmitterShape == EEmitterShape::CONE)
			{
				CreateConeParticleEmitter(emitterID);
			}
			else if (emitterComp.EmitterShape == EEmitterShape::TUBE)
			{
				CreateTubeParticleEmitter(emitterID);
			}

			// Add particle chunk to dirty list
			m_DirtyParticleChunks.PushBack(emitterInstance.ParticleChunk);

			// Create IndirectDrawData
			IndirectData indirectData;
			indirectData.FirstInstance = emitterInstance.ParticleChunk.Offset;
			indirectData.InstanceCount = emitterInstance.ParticleChunk.Size;
			indirectData.FirstIndex = 0;
			indirectData.VertexOffset = 0;
			indirectData.IndexCount = 6;
			m_IndirectData.PushBack(indirectData);

			// Update alive particles buffer
			UpdateAliveParticles();

			// Create EmitterData
			SEmitter emitterData = {};
			emitterData.Color = emitterInstance.Color;
			emitterData.LifeTime = emitterInstance.LifeTime;
			emitterData.Radius = emitterInstance.BeginRadius;
			emitterData.AnimationCount = emitterInstance.AnimationCount;
			emitterData.FirstAnimationIndex = emitterInstance.FirstAnimationIndex;
			emitterData.Gravity = emitterInstance.Gravity;
			emitterData.OneTime = emitterInstance.OneTime;

			// Fetch default texture if none is set
			GUID_Lambda atlasGUID = emitterInstance.AtlasGUID;
			if (atlasGUID == GUID_NONE)
				atlasGUID = m_DefaultAtlasTextureGUID;

			if (!m_AtlasResources.contains(atlasGUID))
			{
				CreateAtlasTextureInstance(atlasGUID, emitterComp.AtlasTileSize);
			}
			emitterData.AtlasIndex = m_AtlasResources[atlasGUID].AtlasIndex;
			m_EmitterData.PushBack(emitterData);

			// Create Transform
			glm::mat4 emitterTransform = glm::translate(positionComp.Position) * glm::toMat4(rotationComp.Quaternion);
			m_EmitterTransformData.PushBack(emitterTransform);

			m_DirtyTransformBuffer = true;
			m_DirtyEmitterBuffer = true;
			m_DirtyIndirectBuffer = true;
			m_DirtyParticleBuffer = true;
			m_DirtyEmitterIndexBuffer = true;
		}

		return true;
	}

	bool ParticleManager::DeactivateEmitterInstance(EmitterID emitterID)
	{
		// Remove indirect draw call
		uint32 removeIndex = emitterID;
		if (removeIndex < m_IndirectData.GetSize())
		{
			auto& emitterInstance = m_Emitters[emitterID];
			uint32 lastIndex = m_IndirectData.GetSize() - 1U;

			// Remove TLAS instances
			uint32 size = m_IndirectData[removeIndex].InstanceCount;
			if (size > 0)
			{
				uint32 offset = m_IndirectData[removeIndex].FirstInstance;
				for (uint32 i = 0; i < size; i++)
				{
					uint32 instanceIndirectIndex = m_ParticleIndexData[offset + i].ASInstanceIndirectIndex;
					VALIDATE(instanceIndirectIndex != UINT32_MAX);
					m_ParticleIndexData[offset + i].ASInstanceIndirectIndex = UINT32_MAX;
					m_pASBuilder->RemoveInstance(instanceIndirectIndex);
				}
			}

			// Replace the removed emitters data with the last emitters data
			m_IndirectData[removeIndex] = m_IndirectData[lastIndex];
			m_EmitterData[removeIndex] = m_EmitterData[lastIndex];
			m_EmitterTransformData[removeIndex] = m_EmitterTransformData[lastIndex];

			// Update particles of last emitter
			size = m_IndirectData[lastIndex].InstanceCount;
			if (size > 0)
			{
				uint32 offset = m_IndirectData[lastIndex].FirstInstance;
				for (uint32 i = 0; i < size; i++)
				{
					m_ParticleIndexData[offset + i].EmitterIndex = removeIndex;
				}
				// Add particle chunk to dirty list
				m_DirtyParticleChunks.PushBack(ParticleChunk{.Offset = offset, .Size = size});
			}
			FreeParticleChunk(emitterInstance.ParticleChunk);
			
			// Replace removed emitter with last emitter
			ReplaceRemovedEmitterWithLast(removeIndex);

			m_IndirectData.PopBack();
			m_EmitterData.PopBack();
			m_EmitterTransformData.PopBack();

			// Update alive particles buffer
			UpdateAliveParticles();
			
			m_DirtyTransformBuffer = true;
			m_DirtyEmitterBuffer = true;
			m_DirtyIndirectBuffer = true;
			m_DirtyEmitterIndexBuffer = true;
		}
		else
		{
			LOG_WARNING("[ParticleManager]: Trying to remove non-existing emitter");
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
				break;
			}
		}

		// TODO: Handle override max capacity particle request
		if (!foundChunk)
			return false;

#if DEBUG_PARTICLE
		LOG_INFO("[ParticleManager]: Allocated Chunk[offset: %u, size: %u]", chunk.Offset, chunk.Size);
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
		LOG_INFO("[ParticleManager]: Freed Chunk: [offset: %u, size : %u]", chunk.Offset, chunk.Size);
		LOG_INFO("[ParticleManager]: Current Free Chunks:");
		for (uint32 i = 0; i < m_FreeParticleChunks.GetSize(); i++)
		{
			LOG_INFO("\tFree chunk%u - [offset: %u, size : %u] ", i, m_FreeParticleChunks[i].Offset, m_FreeParticleChunks[i].Size);
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

			uint32 offset = 0;
			uint32 elementCount = 4;
			CopyDataToBuffer(
				pCommandList,
				(void*)vertices,
				&offset,
				&elementCount,
				1U,
				sizeof(glm::vec4),
				m_ppVertexStagingBuffer,
				&m_pVertexBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Particle Billboard Vertex Buffer");
		}

		// Update Index Buffer
		if (m_DirtyIndexBuffer)
		{
			const uint32 indices[6] =
			{
				2,1,0,
				2,3,1
			};

			uint32 offset = 0;
			uint32 elementCount = 6;
			CopyDataToBuffer(
				pCommandList,
				(void*)indices,
				&offset,
				&elementCount,
				1U,
				sizeof(uint32),
				m_ppIndexStagingBuffer,
				&m_pIndexBuffer,
				FBufferFlag::BUFFER_FLAG_INDEX_BUFFER,
				"Particle Billboard Index Buffer");
		}

		// Update Instance Buffer
		if (m_DirtyIndirectBuffer && m_CreatedDummyBuffer)
		{
			uint32 offset = 0;
			uint32 elementCount = m_IndirectData.GetSize();
			m_DirtyIndirectBuffer = CopyDataToBuffer(
				pCommandList,
				m_IndirectData.GetData(),
				&offset,
				&elementCount,
				1U,
				sizeof(IndirectData),
				m_ppIndirectStagingBuffer,
				&m_pIndirectBuffer,
				FBufferFlag::BUFFER_FLAG_INDIRECT_BUFFER,
				"Particle Instance Buffer");
		}

		{
			// Create offset and buffer size array
			uint32 dirtyChunks = m_DirtyParticleChunks.GetSize();
			TArray<uint32> offsets(dirtyChunks);
			TArray<uint32> elementCounts(dirtyChunks);
			for (uint32 i = 0; i < m_DirtyParticleChunks.GetSize(); i++)
			{
				offsets[i] = m_DirtyParticleChunks[i].Offset;
				elementCounts[i] = m_DirtyParticleChunks[i].Size;
			}

			// Clear all dirty particle chunks
			m_DirtyParticleChunks.Clear();

			// Update Particle Emitter Index buffer
			if (m_DirtyEmitterIndexBuffer && m_CreatedDummyBuffer)
			{
				m_DirtyEmitterIndexBuffer = CopyDataToBuffer(
					pCommandList,
					m_ParticleIndexData.GetData(),
					offsets.GetData(),
					elementCounts.GetData(),
					dirtyChunks,
					sizeof(SParticleIndexData),
					m_ppParticleIndexDataStagingBuffer,
					&m_pParticleIndexDataBuffer,
					FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
					"Emitter indices");
			}

			// Update Particle Instance Buffer
			if (m_DirtyParticleBuffer && m_CreatedDummyBuffer)
			{
				m_DirtyParticleBuffer = CopyDataToBuffer(
					pCommandList,
					m_Particles.GetData(),
					offsets.GetData(),
					elementCounts.GetData(),
					dirtyChunks,
					sizeof(SParticle),
					m_ppParticleStagingBuffer,
					&m_pParticleBuffer,
					FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
					"Particle Instances");
			}
		}

		// Update Emitter Instance Buffer
		if (m_DirtyAliveBuffer && m_CreatedDummyBuffer)
		{
			uint32 offset = 0;
			uint32 elementCount = m_AliveIndices.GetSize();
			m_DirtyAliveBuffer = CopyDataToBuffer(
				pCommandList,
				m_AliveIndices.GetData(),
				&offset,
				&elementCount,
				1U,
				sizeof(uint32),
				m_ppAliveStagingBuffer,
				&m_pAliveBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Alive indices");
		}

		// Update Emitter Instance Buffer
		if (m_DirtyEmitterBuffer && m_CreatedDummyBuffer)
		{
			uint32 offset = 0;
			uint32 elementCount = m_EmitterData.GetSize();
			m_DirtyEmitterBuffer = CopyDataToBuffer(
				pCommandList,
				m_EmitterData.GetData(),
				&offset,
				&elementCount,
				1U,
				sizeof(SEmitter),
				m_ppEmitterStagingBuffer,
				&m_pEmitterBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Emitter Instances");
		}

		// Update Emitter Transform Buffer
		if (m_DirtyTransformBuffer && m_CreatedDummyBuffer)
		{
			uint32 offset = 0;
			uint32 elementCount = m_EmitterTransformData.GetSize();
			m_DirtyTransformBuffer = CopyDataToBuffer(
				pCommandList,
				m_EmitterTransformData.GetData(),
				&offset,
				&elementCount,
				1U,
				sizeof(glm::mat4),
				m_ppTransformStagingBuffer,
				&m_pTransformBuffer,
				FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER,
				"Emitter Transforms");
		}

		// Update Atlas data Buffer
		if (m_DirtyAtlasDataBuffer)
		{
			uint32 offset = 0;
			uint32 elementCount = m_AtlasInfoData.GetSize();
			m_DirtyAtlasDataBuffer = CopyDataToBuffer(
				pCommandList,
				m_AtlasInfoData.GetData(),
				&offset,
				&elementCount,
				1U,
				sizeof(SAtlasInfo),
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

		if (m_DirtyEmitterIndexBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_EMITTER_INDEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pParticleIndexDataBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyEmitterIndexBuffer = false;

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

		if (m_DirtyAliveBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_ALIVE_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pAliveBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyAliveBuffer = false;

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

		if (m_DirtyAtlasTexture)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_ATLAS_IMAGES;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextures = m_AtlasTextures.GetData();
			resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews = m_AtlasTextureViews.GetData();
			resourceUpdateDesc.ExternalTextureUpdate.ppSamplers = m_AtlasSamplers.GetData();
			resourceUpdateDesc.ExternalTextureUpdate.TextureCount = m_AtlasTextures.GetSize();
			resourceUpdateDesc.ExternalTextureUpdate.SamplerCount = m_AtlasSamplers.GetSize();
			pRendergraph->UpdateResource(&resourceUpdateDesc);
		}
		m_DirtyAtlasTexture = false;

		m_CreatedDummyBuffer = true;

		return false;
	}
}