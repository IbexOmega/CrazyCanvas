#include "ECS/Systems/Player/HealthSystemClient.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Events/GameplayEvents.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/Match.h"

/*
* HealthSystemClient
*/

void HealthSystemClient::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();
	ComponentArray<PacketComponent<PacketHealthChanged>>* pHealthChangedComponents = pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

	for (Entity entity : m_HealthEntities)
		{
			HealthComponent& healthComponent = pHealthComponents->GetData(entity);
			PacketComponent<PacketHealthChanged>& packets = pHealthChangedComponents->GetData(entity);
			for (const PacketHealthChanged& packet : packets.GetPacketsReceived())
			{
				if (healthComponent.CurrentHealth != packet.CurrentHealth)
				{
					healthComponent.CurrentHealth = packet.CurrentHealth;
					if (packet.Killed)
					{
						Match::KillPlayer(entity);
						LOG_INFO("PLAYER DIED");
					}

					// Is this the local player
					bool isLocal = false;
					for (Entity playerEntity : m_LocalPlayerEntities)
					{
						if (playerEntity == entity)
						{
							isLocal = true;
							break;
						}
					}

					// Send event to notify systems that a player got hit
					const PositionComponent& positionComp = pECS->GetConstComponent<PositionComponent>(entity);
					const glm::vec3 position = positionComp.Position;

					PlayerHitEvent event(entity, position, isLocal);
					EventQueue::SendEvent(event);
				}
			}
		}
}

bool HealthSystemClient::InitInternal()
{
	return HealthSystem::InitInternal();
}
