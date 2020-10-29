#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include <mutex>

/*
* HealthSystem
*/

HealthSystem HealthSystem::s_Instance;

bool HealthSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_HealthEntities,
				.ComponentAccesses =
				{
					{ RW, HealthComponent::Type() },
					{ RW, PacketComponent<HealthChangedPacket>::Type() }
				}
			}
		};

		// After weaponsystem
		systemReg.Phase = 2;

		RegisterSystem(TYPE_NAME(HealthSystem), systemReg);
	}

	// Register eventhandler
	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HealthSystem::OnProjectileHit);

	return true;
}

void HealthSystem::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();
	ComponentArray<PacketComponent<HealthChangedPacket>>* pHealthChangedComponents = pECS->GetComponentArray<PacketComponent<HealthChangedPacket>>();

	if (MultiplayerUtils::IsServer())
	{
		// Since the events are not sent threadsafe
		{
			std::scoped_lock<SpinLock> lock(m_DeferredEventsLock);
			if (!m_DeferredHitEvents.IsEmpty())
			{
				m_EventsToProcess = m_DeferredHitEvents;
				m_DeferredHitEvents.Clear();
			}
		}

		if (!m_EventsToProcess.IsEmpty())
		{
			for (ProjectileHitEvent& event : m_EventsToProcess)
			{
				// CollisionInfo1 is the entity that got hit
				Entity entity = event.CollisionInfo1.Entity;
				EAmmoType ammoType = event.AmmoType;

				//LOG_INFO("Retriving health from entity=%d", entity);

				HealthComponent& healthComponent = pHealthComponents->GetData(entity);
				PacketComponent<HealthChangedPacket>& packets = pHealthChangedComponents->GetData(entity);
				if (healthComponent.CurrentHealth > 0)
				{
					if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
					{
						healthComponent.CurrentHealth -= 10;
						LOG_INFO("Player damaged. Health=%d", healthComponent.CurrentHealth);
					}
					else if (ammoType == EAmmoType::AMMO_TYPE_WATER)
					{
						healthComponent.CurrentHealth += 10;
						LOG_INFO("Player got splashed. Health=%d", healthComponent.CurrentHealth);
					}

					// Set min/max health 
					if (healthComponent.CurrentHealth <= 0)
					{
						LOG_INFO("PLAYER DIED");
						healthComponent.CurrentHealth = 0;

						PlayerDiedEvent diedEvent(entity);
						EventQueue::SendEvent(diedEvent);
					}
					else if (healthComponent.CurrentHealth > 100)
					{
						LOG_INFO("PLAYER REACHED FULL HEALTH");
						healthComponent.CurrentHealth = 100;
					}

					HealthChangedPacket packet = {};
					packet.CurrentHealth = healthComponent.CurrentHealth;
					packets.SendPacket(packet);
				}
			}

			m_EventsToProcess.Clear();
		}
	}
	else
	{
		for (Entity entity : m_HealthEntities)
		{
			HealthComponent& healthComponent = pHealthComponents->GetData(entity);
			PacketComponent<HealthChangedPacket>& packets = pHealthChangedComponents->GetData(entity);
			for (const HealthChangedPacket& packet : packets.GetPacketsReceived())
			{
				//LOG_INFO("GOT A HEALTH PACKAGE");

				if (healthComponent.CurrentHealth != packet.CurrentHealth)
				{
					//LOG_INFO("Health changed old=%d new=%d", healthComponent.CurrentHealth, packet.CurrentHealth);
					healthComponent.CurrentHealth = packet.CurrentHealth;
				}
			}
		}
	}
}

bool HealthSystem::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;
	
	if (MultiplayerUtils::IsServer())
	{
		std::scoped_lock<SpinLock> lock(m_DeferredEventsLock);
		//LOG_INFO("Something got hit CollisionInfo0=%d, CollisionInfo1=%d", projectileHitEvent.CollisionInfo0.Entity, projectileHitEvent.CollisionInfo1.Entity);

		for (Entity entity : m_HealthEntities)
		{
			//LOG_INFO("....Entity: %d", entity);

			// CollisionInfo0 is the projectile
			if (projectileHitEvent.CollisionInfo1.Entity == entity)
			{
				m_DeferredHitEvents.EmplaceBack(projectileHitEvent);
				//LOG_INFO("Player got hit");
				break;
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}