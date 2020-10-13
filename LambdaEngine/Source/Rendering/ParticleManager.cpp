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
	void ParticleManager::Init(uint32 maxParticles)
	{
		m_Particles.Reserve(maxParticles);
	}

	void ParticleManager::Release()
	{
	}

	void ParticleManager::Tick(Timestamp deltaTime, uint32 modFrameIndex)
	{
		m_ModFrameIndex = modFrameIndex;
	}

	void ParticleManager::OnEmitterEntityAdded(Entity entity)
	{
		ECSCore* ecsCore = ECSCore::GetInstance();
		PositionComponent positionComp			= ecsCore->GetComponent<PositionComponent>(entity);
		RotationComponent rotationComp			= ecsCore->GetComponent<RotationComponent>(entity);
		ParticleEmitterComponent emitterComp	= ecsCore->GetComponent<ParticleEmitterComponent>(entity);
		
		ParticleEmitterInstance instance = {};
		instance.position		= positionComp.Position;
		instance.rotation		= rotationComp.Quaternion;
		instance.ParticleCount	= emitterComp.ParticleCount;
		instance.Angle			= emitterComp.Angle;
		instance.Velocity		= emitterComp.Velocity;
		instance.Acceleration	= emitterComp.Acceleration;
		instance.LifeTime		= emitterComp.LifeTime;
		instance.ParticleRadius	= emitterComp.ParticleRadius;
		if (emitterComp.EmitterShape == EEmitterShape::CONE)
		{
			CreateConeParticleEmitter(instance);
		}
	}

	void ParticleManager::OnEmitterEntityRemoved(Entity entity)
	{
	}

	void ParticleManager::CreateConeParticleEmitter(ParticleEmitterInstance& emitterInstance)
	{
		emitterInstance.ParticleOffset = m_Particles.GetSize();

		const glm::vec3 forward		= GetForward(emitterInstance.rotation);
		const glm::vec3 up			= GetUp(emitterInstance.rotation);
		const glm::vec3 right		= GetRight(emitterInstance.rotation);
		const float		halfAngle	= emitterInstance.Angle * 0.5f;

		for (uint32 i = 0; i < emitterInstance.ParticleCount; i++)
		{
			SParticle particle;

			glm::vec3 direction = forward;

			direction = glm::rotate(direction, Random::Float32(-glm::radians(halfAngle), glm::radians(halfAngle)), up);
			direction = glm::rotate(direction, Random::Float32(-glm::radians(halfAngle), glm::radians(halfAngle)), right);

			direction = glm::normalize(direction);

			particle.Transform		= glm::translate(emitterInstance.position);
			particle.Color			= glm::vec4(1.0f);
			particle.Velocity		= direction * emitterInstance.Velocity;
			particle.Acceleration	= direction * emitterInstance.Acceleration;
			particle.LifeTime		= emitterInstance.LifeTime;
			particle.Radius			= emitterInstance.ParticleRadius;

			m_Particles.PushBack(particle);
		}
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
		// Update Vertex Buffer
		if (m_DirtyVertexBuffer)
		{
			uint32 requiredBufferSize = m_Particles.GetSize();

			Buffer* pStagingBuffer = m_ppVertexStagingBuffer[m_ModFrameIndex];

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = "Particle Vertex Staging Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes = requiredBufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				m_ppVertexStagingBuffer[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, m_Particles.GetData(), requiredBufferSize);
			pStagingBuffer->Unmap();

			if (m_pVertexBuffer == nullptr || m_pVertexBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (m_pVertexBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pVertexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = "Particle Vertex Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				bufferDesc.SizeInBytes = requiredBufferSize;

				m_pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pParticleBuffer, 0, requiredBufferSize);

			m_DirtyVertexBuffer = false;
		}
		else
		{
			m_DirtyVertexBuffer = false; // Only update resource when buffer is recreated
		}

		// Update Particle Instance Buffer
		if (m_DirtyParticleBuffer)
		{
			uint32 requiredBufferSize = m_Particles.GetSize();

			Buffer* pStagingBuffer = m_ppParticleStagingBuffer[m_ModFrameIndex];

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = "Particle Instance Staging Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes = requiredBufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				m_ppParticleStagingBuffer[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, m_Particles.GetData(), requiredBufferSize);
			pStagingBuffer->Unmap();

			if (m_pParticleBuffer == nullptr || m_pParticleBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (m_pParticleBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pParticleBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = "Particle Instance Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				bufferDesc.SizeInBytes = requiredBufferSize;

				m_pParticleBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}
			else
			{
				m_DirtyParticleBuffer = false; // Only update resource when buffer is recreated
			}

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pParticleBuffer, 0, requiredBufferSize);

		}

		return false;
	}
	bool ParticleManager::UpdateResources(RenderGraph* pRendergraph)
	{
		if (m_DirtyVertexBuffer)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = SCENE_PARTICLE_VERTEX_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &m_pVertexBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
			pRendergraph->UpdateResource(&resourceUpdateDesc);

			m_DirtyVertexBuffer = false;
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

