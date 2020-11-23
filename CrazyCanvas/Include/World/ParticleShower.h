#pragma once

#include "ECS/Entity.h"
#include "ECS/ECSCore.h"

#include "RenderStages/PaintMaskRenderer.h"
#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Component.h"
#include "ECS/ComponentOwner.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"

struct ParticleShowerComponent
{
	DECL_COMPONENT(ParticleShowerComponent);
	LambdaEngine::Timestamp ShowerAvailableTimestamp;
	LambdaEngine::Timestamp ShowerCooldown;
};

// ParticleShowerCallback is called when an entity intersect with a bounding box. Entity0 is the particle shower.
inline void ParticleShowerCallback(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	UNREFERENCED_VARIABLE(entity0);
	//LOG_INFO("Entity %d is taking a shower", entity1);

	LambdaEngine::ECSCore* pECS = LambdaEngine::ECSCore::GetInstance();
	ParticleShowerComponent& showerComponent = pECS->GetComponent<ParticleShowerComponent>(entity1);

	if (LambdaEngine::EngineLoop::GetTimeSinceStart() > showerComponent.ShowerAvailableTimestamp)
	{
		LambdaEngine::PaintMaskRenderer::ResetServer(entity1);
		LambdaEngine::PaintMaskRenderer::ResetServer(pECS->GetComponent<LambdaEngine::ChildComponent>(entity1).GetEntityWithTag("weapon"));
		HealthSystemServer::ResetHealth(entity1);

		showerComponent.ShowerAvailableTimestamp = LambdaEngine::EngineLoop::GetTimeSinceStart() + showerComponent.ShowerCooldown;
	}
}
