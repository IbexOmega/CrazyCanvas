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
		SAFEDELETE(m_pIndirectBuffer);
		SAFEDELETE(m_pVertexBuffer);
		SAFEDELETE(m_pIndexBuffer);
		SAFEDELETE(m_pParticleBuffer);
	}

	void ParticleManager::Tick(Timestamp deltaTime, uint32 modFrameIndex)
	{
		m_ModFrameIndex = modFrameIndex;
	}

	void ParticleManager::OnEmitterEntityAdded(Entity entity)
	{
		ECSCore* ecsCore = ECSCore::GetInstance();
		PositionComponent positionComp = ecsCore->GetComponent<PositionComponent>(entity);
		RotationComponent rotationComp = ecsCore->GetComponent<RotationComponent>(entity);
		ParticleEmitterComponent emitterComp = ecsCore->GetComponent<ParticleEmitterComponent>(entity);

		ParticleEmitterInstance instance = {};
		instance.position = positionComp.Position;
		instance.rotation = rotationComp.Quaternion;
		instance.ParticleCount = emitterComp.ParticleCount;
		instance.Angle = emitterComp.Angle;
		instance.Velocity = emitterComp.Velocity;
		instance.Acceleration = emitterComp.Acceleration;
		instance.LifeTime = emitterComp.LifeTime;
		instance.ParticleRadius = emitterComp.ParticleRadius;

		if (emitterComp.EmitterShape == EEmitterShape::CONE)
		{
			CreateConeParticleEmitter(instance);
		}

		IndirectData indirectData;
		indirectData.firstInstance = instance.ParticleOffset;
		indirectData.instanceCount = instance.ParticleCount;
		indirectData.firstIndex = 0;
		indirectData.indexCount = 6;
		m_IndirectData.PushBack(indirectData);

		m_DirtyIndirectBuffer = true;
		m_DirtyParticleBuffer = true;
		m_DirtyIndexBuffer = true;
	}

	void ParticleManager::OnEmitterEntityRemoved(Entity entity)
	{
	}

	void ParticleManager::CreateConeParticleEmitter(ParticleEmitterInstance& emitterInstance)
	{
		emitterInstance.ParticleOffset = m_Particles.GetSize();

		const glm::vec3 forward = GetForward(emitterInstance.rotation);
		const glm::vec3 up = GetUp(emitterInstance.rotation);
		const glm::vec3 right = GetRight(emitterInstance.rotation);
		const float		halfAngle = emitterInstance.Angle * 0.5f;

		for (uint32 i = 0; i < emitterInstance.ParticleCount; i++)
		{
			SParticle particle;

			glm::vec3 direction = forward;

			direction = glm::rotate(direction, Random::Float32(-glm::radians(halfAngle), glm::radians(halfAngle)), up);
			direction = glm::rotate(direction, Random::Float32(-glm::radians(halfAngle), glm::radians(halfAngle)), right);

			direction = glm::normalize(direction);

			particle.Transform = glm::translate(emitterInstance.position);
			particle.Color = glm::vec4(1.0f);
			particle.Velocity = direction * emitterInstance.Velocity;
			particle.Acceleration = direction * emitterInstance.Acceleration;
			particle.LifeTime = emitterInstance.LifeTime;
			particle.Radius = emitterInstance.ParticleRadius;

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
		// Update Instance Buffer
		if (m_DirtyIndirectBuffer)
		{
			uint32 requiredBufferSize = m_IndirectData.GetSize() * sizeof(IndirectData);

			Buffer* pStagingBuffer = m_ppIndirectStagingBuffer[m_ModFrameIndex];

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = "Particle Indirect Staging Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes = requiredBufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				m_ppIndirectStagingBuffer[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, m_IndirectData.GetData(), requiredBufferSize);
			pStagingBuffer->Unmap();

			if (m_pIndirectBuffer == nullptr || m_pIndirectBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (m_pIndirectBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pIndirectBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = "Particle Indirect Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDIRECT_BUFFER;
				bufferDesc.SizeInBytes = requiredBufferSize;

				m_pIndirectBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pIndirectBuffer, 0, requiredBufferSize);
		}


		// Update Vertex Buffer
		if (m_DirtyVertexBuffer)
		{
			uint32 requiredBufferSize = 0;

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

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pVertexBuffer, 0, requiredBufferSize);
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

			Buffer* pStagingBuffer = m_ppIndexStagingBuffer[m_ModFrameIndex];

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = "Particle Index Staging Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes = requiredBufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				m_ppIndexStagingBuffer[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, indices, requiredBufferSize);
			pStagingBuffer->Unmap();

			if (m_pIndexBuffer == nullptr || m_pIndexBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (m_pIndexBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pIndexBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName = "Particle Index Buffer";
				bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER;
				bufferDesc.SizeInBytes = requiredBufferSize;

				m_pIndexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pIndexBuffer, 0, requiredBufferSize);
		}
		else
		{
			m_DirtyIndexBuffer = false; // Only update resource when buffer is recreated
		}

		// Update Particle Instance Buffer
		if (m_DirtyParticleBuffer)
		{
			uint32 requiredBufferSize = m_Particles.GetSize() * sizeof(SParticle);

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