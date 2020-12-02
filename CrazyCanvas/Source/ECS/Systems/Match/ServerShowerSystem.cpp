#include "ECS/Components/Match/ShowerComponent.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Systems/Match/ServerShowerSystem.h"
#include "ECS/Systems/Player/HealthSystemServer.h"

#include "ECS/ComponentArray.h"

#include "MeshPaint/MeshPaintHandler.h"

#include "Multiplayer/Packet/PacketResetPlayerTexture.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Team/TeamComponent.h"

ServerShowerSystem::ServerShowerSystem() {

}

ServerShowerSystem::~ServerShowerSystem() {

}

void ServerShowerSystem::OnPlayerShowerCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	UNREFERENCED_VARIABLE(entity0);

	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	const TeamComponent& playerTeamComponent = pECS->GetConstComponent<TeamComponent>(entity1);
	ShowerComponent& playerShowerComponent = pECS->GetComponent<ShowerComponent>(entity1);

	bool validCollision = false;
	TeamComponent showerTeamComponent;
	if (pECS->GetConstComponentIf<TeamComponent>(entity0, showerTeamComponent))
	{
		validCollision = (showerTeamComponent.TeamIndex == playerTeamComponent.TeamIndex);
	}
	else
	{
		//If the shower has no Team Component then everyone can shower in it.
		validCollision = true;
	}

	if (validCollision)
	{
		// If player has full health, do not use the shower
		ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();
		HealthComponent& healthComponent = pHealthComponents->GetData(entity1);

		if (healthComponent.CurrentHealth == START_HEALTH)
			return;

		// Check if player has recently used the shower
		if (EngineLoop::GetTimeSinceStart() > playerShowerComponent.ShowerAvailableTimestamp)
		{
			MeshPaintHandler::ResetServer(entity1);
			MeshPaintHandler::ResetServer(pECS->GetComponent<ChildComponent>(entity1).GetEntityWithTag("weapon"));
			HealthSystemServer::ResetHealth(entity1);

			playerShowerComponent.ShowerAvailableTimestamp = EngineLoop::GetTimeSinceStart() + playerShowerComponent.ShowerCooldown;

			// Send package to client to reset texture
			PacketComponent<PacketResetPlayerTexture>& packets = pECS->GetComponent<PacketComponent<PacketResetPlayerTexture>>(entity1);
			PacketResetPlayerTexture packet = {};
			packets.SendPacket(packet);
		}
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