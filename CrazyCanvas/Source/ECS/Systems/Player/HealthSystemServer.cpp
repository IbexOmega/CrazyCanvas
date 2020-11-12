#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/Match.h"

#include "Lobby/PlayerManagerServer.h"

#include <mutex>

/*
* HealthSystemServer
*/

void HealthSystemServer::ResetEntityHealth(LambdaEngine::Entity entityToReset)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_DeferredResetsLock);
	for (Entity entity : m_HealthEntities)
	{
		if (entityToReset == entity)
		{
			m_DeferredResets.EmplaceBack(entityToReset);
			break;
		}
	}
}

void HealthSystemServer::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();
	ComponentArray<PacketComponent<PacketHealthChanged>>* pHealthChangedComponents = pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

	// More threadsafe
	{
		std::scoped_lock<SpinLock> lock(m_DeferredEventsLock);
		if (!m_DeferredHitEvents.IsEmpty())
		{
			m_EventsToProcess = m_DeferredHitEvents;
			m_DeferredHitEvents.Clear();
		}
	}

	// More threadsafe
	{
		std::scoped_lock<SpinLock> lock(m_DeferredResetsLock);
		if (!m_DeferredResets.IsEmpty())
		{
			// Reset health
			for (Entity entity : m_DeferredResets)
			{
				HealthComponent& healthComponent = pECS->GetComponent<HealthComponent>(entity);
				PacketComponent<PacketHealthChanged>& packets = pECS->GetComponent<PacketComponent<PacketHealthChanged>>(entity);
				healthComponent.CurrentHealth = START_HEALTH;

				PacketHealthChanged packet = {};
				packet.CurrentHealth = healthComponent.CurrentHealth;
				packets.SendPacket(packet);
			}

			m_DeferredResets.Clear();
		}
	}

	if (!m_EventsToProcess.IsEmpty())
	{
		//ComponentArray<ProjectileComponent>* pProjectileComponents = pECS->GetComponentArray<ProjectileComponent>();

		for (ProjectileHitEvent& event : m_EventsToProcess)
		{
			// CollisionInfo1 is the entity that got hit
			Entity entity = event.CollisionInfo1.Entity;
			EAmmoType ammoType = event.AmmoType;

			LOG_INFO("Retrived ProjectileHitEvent");

			HealthComponent& healthComponent = pHealthComponents->GetData(entity);
			PacketComponent<PacketHealthChanged>& packets = pHealthChangedComponents->GetData(entity);
			if (healthComponent.CurrentHealth > 0)
			{
				if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
				{
					healthComponent.CurrentHealth -= HIT_DAMAGE;
					if (healthComponent.CurrentHealth <= 0)
					{
						/*Entity entityProjectile = event.CollisionInfo0.Entity;
						const ProjectileComponent& projectileComponent = pProjectileComponents->GetConstData(entityProjectile);

						Match::KillPlayer(entity, projectileComponent.Owner);*/
						Match::KillPlayer(entity, UINT32_MAX);
					}

					LOG_INFO("Player damaged. Health=%d", healthComponent.CurrentHealth);
				}
				else if (ammoType == EAmmoType::AMMO_TYPE_WATER)
				{
					healthComponent.CurrentHealth = std::min(healthComponent.CurrentHealth + HIT_DAMAGE, 100);
					if (healthComponent.CurrentHealth >= 100)
					{
						LOG_INFO("PLAYER REACHED FULL HEALTH");
					}

					LOG_INFO("Player got splashed. Health=%d", healthComponent.CurrentHealth);
				}

				PacketHealthChanged packet = {};
				packet.CurrentHealth = healthComponent.CurrentHealth;
				packets.SendPacket(packet);
			}
		}

		m_EventsToProcess.Clear();
	}
}

bool HealthSystemServer::InitInternal()
{
	using namespace LambdaEngine;

	// Register system
	{
		SystemRegistration systemReg = {};
		HealthSystem::CreateBaseSystemRegistration(systemReg);
		RegisterSystem(TYPE_NAME(HealthSystemServer), systemReg);
	}

	// Register eventhandler
	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HealthSystemServer::OnProjectileHit);
	return true;
}

bool HealthSystemServer::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_DeferredEventsLock);
	// LOG_INFO("Something got hit CollisionInfo0=%d, CollisionInfo1=%d", projectileHitEvent.CollisionInfo0.Entity, projectileHitEvent.CollisionInfo1.Entity);

	for (Entity entity : m_HealthEntities)
	{
		// CollisionInfo0 is the projectile
		if (projectileHitEvent.CollisionInfo1.Entity == entity)
		{
			m_DeferredHitEvents.EmplaceBack(projectileHitEvent);
			break;
		}
	}

	return true;
}
