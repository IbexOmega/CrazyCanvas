#pragma once

#include "ECS/Entity.h"
#include "ECS/ECSCore.h"

#include "Rendering/PaintMaskRenderer.h"
#include "ECS/Systems/Player/HealthSystemServer.h"

struct ParticleShowerComponent
{
	DECL_COMPONENT(ParticleShowerComponent);
	LambdaEngine::Timestamp ShowerAvailableTimestamp;
	LambdaEngine::Timestamp ShowerCooldown;
};

// KillPlaneCallback is called when an entity falls behind a kill plane. Entity0 is the kill plane entity.
inline void ParticleShowerCallback(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	//UNREFERENCED_VARIABLE(entity0);
	LOG_INFO("Entity %d is taking a shower", entity1);

	LambdaEngine::ECSCore* pECS = LambdaEngine::ECSCore::GetInstance();
	ParticleShowerComponent& showerComponent = pECS->GetComponent<ParticleShowerComponent>(entity0);
	//PacketComponent<PacketFlagEdited>& flagPacketComponent = pECS->GetComponent<PacketComponent<PacketFlagEdited>>(entity0);

	if (LambdaEngine::EngineLoop::GetTimeSinceStart() > showerComponent.ShowerAvailableTimestamp)
	{
		LambdaEngine::PaintMaskRenderer::ResetServer(entity1);
		HealthSystemServer::ResetHealth(entity1);

		showerComponent.ShowerAvailableTimestamp = LambdaEngine::EngineLoop::GetTimeSinceStart() + showerComponent.ShowerCooldown;
	}

}
