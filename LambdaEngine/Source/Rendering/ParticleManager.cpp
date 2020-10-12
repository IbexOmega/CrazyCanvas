#include "Rendering/ParticleManager.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Components/Physics/Transform.h"

namespace LambdaEngine {

	void ParticleManager::Init()
	{
	}

	void ParticleManager::Release()
	{
	}

	void ParticleManager::OnEmitterEntityAdded(Entity entity)
	{
		ECSCore* ecsCore = ECSCore::GetInstance();
		PositionComponent positionComp			= ecsCore->GetComponent<PositionComponent>(entity);
		RotationComponent rotationComp			= ecsCore->GetComponent<RotationComponent>(entity);
		ParticleEmitterComponent emitterComp	= ecsCore->GetComponent<ParticleEmitterComponent>(entity);


	}
	void ParticleManager::OnEmitterEntityRemoved(Entity entity)
	{
	}
}
