#include "ECS/Components/Match/ShowerComponent.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Systems/Match/ServerShowerSystem.h"
#include "ECS/Systems/Player/HealthSystemServer.h"

#include "ECS/ComponentArray.h"

#include "RenderStages/PaintMaskRenderer.h"

#include "Multiplayer/Packet/PacketResetPlayerTexture.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"

ServerShowerSystem::ServerShowerSystem() {

}

ServerShowerSystem::~ServerShowerSystem() {

}

void ServerShowerSystem::OnPlayerShowerCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	UNREFERENCED_VARIABLE(entity0);

	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	ShowerComponent& showerComponent = pECS->GetComponent<ShowerComponent>(entity1);
	LOG_INFO("Entity %d is taking a shower", entity1);

	// If player has full health, do not use the shower
	ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();
	HealthComponent& healthComponent = pHealthComponents->GetData(entity1);

	if (healthComponent.CurrentHealth == START_HEALTH)
		return;

	// Check if player has recently used the shower
	if (EngineLoop::GetTimeSinceStart() > showerComponent.ShowerAvailableTimestamp)
	{
		LOG_INFO("Entity %d is taking a shower", entity1);

		PaintMaskRenderer::ResetServer(entity1);
		PaintMaskRenderer::ResetServer(pECS->GetComponent<ChildComponent>(entity1).GetEntityWithTag("weapon"));
		HealthSystemServer::ResetHealth(entity1);

		showerComponent.ShowerAvailableTimestamp = EngineLoop::GetTimeSinceStart() + showerComponent.ShowerCooldown;

		// Send package to client to reset texture
		PacketComponent<PacketResetPlayerTexture>& packets = pECS->GetComponent<PacketComponent<PacketResetPlayerTexture>>(entity1);
		PacketResetPlayerTexture packet = {};
		packets.SendPacket(packet);
	}
}

void ServerShowerSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}

void ServerShowerSystem::FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}