#pragma once

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/ParticleEmitter.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine 
{
	class Buffer;
	class CommandList;
	class RenderGraph;

	struct ParticleEmitterInstance
	{
		glm::vec3		position;
		glm::quat		rotation;
		float			Angle = 90.f;
		float			Velocity;
		float			Acceleration;
		float			LifeTime;
		float			ParticleRadius;
		uint32			ParticleOffset = 0U;
		uint32			ParticleCount;
	};

	struct SParticle
	{
		glm::mat4 Transform;
		glm::vec4 Color;
		glm::vec3 Velocity;
		glm::vec3 Acceleration;
		float LifeTime;
		float Radius;
	};

	class ParticleManager
	{
	public:
		ParticleManager() = default;
		~ParticleManager() = default;

		void Init(uint32 maxParticles);
		void Release();

		void Tick(Timestamp deltaTime, uint32 modFrameIndex);

		void OnEmitterEntityAdded(Entity entity);
		void OnEmitterEntityRemoved(Entity entity);

		bool UpdateBuffers(CommandList* pCommandList);
		bool UpdateResources(RenderGraph* pRendergraph);
	private:
		void CreateConeParticleEmitter(ParticleEmitterInstance& emitterInstance);
		
		void CleanBuffers();
	private:
		uint32						m_ModFrameIndex;

		bool						m_DirtyParticleBuffer;
		bool						m_DirtyVertexBuffer;

		Buffer*						m_ppVertexStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*						m_pVertexBuffer = nullptr;

		Buffer*						m_ppParticleStagingBuffer[BACK_BUFFER_COUNT] = { nullptr };
		Buffer*						m_pParticleBuffer = nullptr;

		TArray<DeviceChild*>		m_ResourcesToRemove[BACK_BUFFER_COUNT];

		TArray<SParticle>			m_Particles;

		THashTable<Entity, ParticleEmitterInstance>	m_ActiveEmitters;
		THashTable<Entity, ParticleEmitterInstance>	m_SleepingEmitters;
	};
}