#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/Match.h"

#include <mutex>

#define HIT_DAMAGE 20

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
					{ RW, PacketComponent<PacketHealthChanged>::Type() }
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

void HealthSystem::ResetEntityHealth(LambdaEngine::Entity entityToReset)
{
	using namespace LambdaEngine;

	if (MultiplayerUtils::IsServer())
	{
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
}

void HealthSystem::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();
	ComponentArray<PacketComponent<PacketHealthChanged>>* pHealthChangedComponents = pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

	if (MultiplayerUtils::IsServer())
	{
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
					HealthComponent& healthComponent				= pECS->GetComponent<HealthComponent>(entity);
					PacketComponent<PacketHealthChanged>& packets	= pECS->GetComponent<PacketComponent<PacketHealthChanged>>(entity);
					healthComponent.CurrentHealth = START_HEALTH;

					PacketHealthChanged packet = {};
					packet.CurrentHealth	= healthComponent.CurrentHealth;
					packet.Killed			= false;
					packets.SendPacket(packet);
				}

				m_DeferredResets.Clear();
			}
		}

		if (!m_EventsToProcess.IsEmpty())
		{
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
					bool killed = false;
					if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
					{
						healthComponent.CurrentHealth -= HIT_DAMAGE;
						if (healthComponent.CurrentHealth <= 0)
						{
							Match::KillPlayer(entity);
							killed = true;

							LOG_INFO("PLAYER DIED");
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
					packet.Killed = killed;
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
			PacketComponent<PacketHealthChanged>& packets = pHealthChangedComponents->GetData(entity);
			for (const PacketHealthChanged& packet : packets.GetPacketsReceived())
			{
				if (healthComponent.CurrentHealth != packet.CurrentHealth)
				{
					LOG_INFO("Health changed old=%d new=%d", healthComponent.CurrentHealth, packet.CurrentHealth);
					healthComponent.CurrentHealth = packet.CurrentHealth;
					if (packet.Killed)
					{
						Match::KillPlayer(entity);
						LOG_INFO("PLAYER DIED");
					}
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
		LOG_INFO("Something got hit CollisionInfo0=%d, CollisionInfo1=%d", projectileHitEvent.CollisionInfo0.Entity, projectileHitEvent.CollisionInfo1.Entity);

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
	else
	{
		return false;
	}
}
