#pragma once

#include "ECS/Entity.h"
#include "ECS/ECSCore.h"
#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Component.h"

#include "ECS/Components/Match/ShowerComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/Multiplayer/MultiplayerUtils.h"

#include "RenderStages/PaintMaskRenderer.h"


// ParticleShowerCallback is called when an entity intersect with a bounding box. Entity0 is the particle shower.
inline void ParticleShowerCallback(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	UNREFERENCED_VARIABLE(entity0);

	LambdaEngine::ECSCore* pECS = LambdaEngine::ECSCore::GetInstance();

	ShowerComponent& showerComponent = pECS->GetComponent<ShowerComponent>(entity1);

	if (LambdaEngine::EngineLoop::GetTimeSinceStart() > showerComponent.ShowerAvailableTimestamp)
	{
		LOG_INFO("Entity %d is taking a shower", entity1);

		if (!LambdaEngine::MultiplayerUtils::IsServer())
		{
			LambdaEngine::PaintMaskRenderer::ResetServer(entity1);
			LambdaEngine::PaintMaskRenderer::ResetServer(pECS->GetComponent<LambdaEngine::ChildComponent>(entity1).GetEntityWithTag("weapon"));
		}
		else
			HealthSystemServer::ResetHealth(entity1);

		showerComponent.ShowerAvailableTimestamp = LambdaEngine::EngineLoop::GetTimeSinceStart() + showerComponent.ShowerCooldown;
	}
}