#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/PlayerEvents.h"

#include <mutex>

HealthSystem::HealthSystem()
	: m_HealthEntities()
{
}

HealthSystem::~HealthSystem()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<ProjectileHitEvent>(this, &HealthSystem::OnProjectileHit);
}

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
					{ RW, HealthComponent::Type() }
				}
			}
		};

		// After weaponsystem -> Do not know if this is correct
		systemReg.Phase = 2;

		RegisterSystem(systemReg);
	}

	// Register eventhandler
	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HealthSystem::OnProjectileHit);

	return true;
}

void HealthSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();

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

			LOG_INFO("Retriving health from entity=%d", entity);

			HealthComponent& healthComponent = pHealthComponents->GetData(entity);
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

				if (healthComponent.CurrentHealth <= 0)
				{
					LOG_INFO("PLAYER DIED");
					healthComponent.CurrentHealth = 0;

					PlayerDiedEvent event(entity);
					EventQueue::SendEvent(event);
				}
			}
		}
		m_EventsToProcess.Clear();
	}
}

bool HealthSystem::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_DeferredEventsLock);
	LOG_INFO("Something got hit CollisionInfo0=%d, CollisionInfo1=%d", projectileHitEvent.CollisionInfo0.Entity, projectileHitEvent.CollisionInfo1.Entity);

	for (Entity entity : m_HealthEntities)
	{
		LOG_INFO("....Entity: %d", entity);

		// CollisionInfo0 is the projectile
		if (projectileHitEvent.CollisionInfo1.Entity == entity)
		{
			m_DeferredHitEvents.EmplaceBack(projectileHitEvent);
			LOG_INFO("Player got hit");
			break;
		}
	}

	return true;
}