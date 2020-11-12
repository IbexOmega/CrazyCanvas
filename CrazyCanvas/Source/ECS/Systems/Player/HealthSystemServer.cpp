#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/Match.h"

#include "EventHandlers/MeshPaintHandler.h"

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
	ComponentArray<HealthComponent>*	pHealthComponents		= pECS->GetComponentArray<HealthComponent>();
	ComponentArray<MeshPaintComponent>* pMeshPaintComponents	= pECS->GetComponentArray<MeshPaintComponent>();
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
			Entity		entity	 = event.CollisionInfo1.Entity;
			EAmmoType	ammoType = event.AmmoType;

			HealthComponent& healthComponent				= pHealthComponents->GetData(entity);
			PacketComponent<PacketHealthChanged>& packets	= pHealthChangedComponents->GetData(entity);

			// Check painted pixels
			if (pMeshPaintComponents->HasComponent(entity))
			{
				MeshPaintComponent& meshPaintComponent = pMeshPaintComponents->GetData(entity);
				Buffer* pBuffer = meshPaintComponent.pReadBackBuffer;

				struct Pixel
				{
					byte Red;
					byte Green;
				};

				Pixel* pPaintMask = reinterpret_cast<Pixel*>(pBuffer->Map());
				VALIDATE(pPaintMask != nullptr);

				constexpr uint32 SIZE_Y = 32;
				constexpr uint32 SIZE_X = SIZE_Y;
				
				uint32 paintedPixels = 0;
				for (uint32 y = 0; y < SIZE_Y; y++)
				{
					for (uint32 x = 0; x < SIZE_X; x++)
					{
						const byte mask		 = pPaintMask[(y * SIZE_X) + x].Red;
						const bool isPainted = IS_MASK_PAINTED(mask);
						if (isPainted)
						{
							paintedPixels++;
						}
					}
				}

				constexpr float32 BIASED_MAX_HEALTH = 0.15f;
				constexpr float32 MAX_PIXELS		= float32(SIZE_Y * SIZE_X * BIASED_MAX_HEALTH);
				constexpr float32 START_HEALTH_F	= float32(START_HEALTH);
				const float32 paintedHealth		= float32(paintedPixels) / float32(MAX_PIXELS);
				healthComponent.CurrentHealth	= std::min<uint32>(int32(START_HEALTH_F * (1.0f - paintedHealth)), START_HEALTH);

				LOG_INFO("PLAYER HEALTH: CurrentHealth=%u, paintedHealth=%.4f paintedPixels=%u MAX_PIXELS=%.4f", 
					healthComponent.CurrentHealth, 
					paintedHealth, 
					paintedPixels, 
					MAX_PIXELS);

				pBuffer->Unmap();
			}
			else
			{
				VALIDATE(false);

				if (healthComponent.CurrentHealth > 0)
				{
					if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
					{
						healthComponent.CurrentHealth = std::min(healthComponent.CurrentHealth - HIT_DAMAGE, START_HEALTH);
						LOG_INFO("Player damaged. Health=%d", healthComponent.CurrentHealth);
					}
					else if (ammoType == EAmmoType::AMMO_TYPE_WATER)
					{
						healthComponent.CurrentHealth = std::min(healthComponent.CurrentHealth + HIT_DAMAGE, START_HEALTH);
						if (healthComponent.CurrentHealth >= 100)
						{
							LOG_INFO("PLAYER REACHED FULL HEALTH");
						}

						LOG_INFO("Player got splashed. Health=%d", healthComponent.CurrentHealth);
					}
				}
			}

			// Check if killed
			bool killed = false;
			if (healthComponent.CurrentHealth <= 0)
			{
				Match::KillPlayer(entity);
				killed = true;

				LOG_INFO("PLAYER DIED");
			}

			PacketHealthChanged packet = {};
			packet.CurrentHealth	= healthComponent.CurrentHealth;
			packet.Killed			= killed;
			packets.SendPacket(packet);
		}

		m_EventsToProcess.Clear();
	}
}

bool HealthSystemServer::InitInternal()
{
	using namespace LambdaEngine;

	if (!HealthSystem::InitInternal())
	{
		return false;
	}

	EntitySubscriberRegistration subscription = {};
	subscription.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_MeshPaintEntities,
			.ComponentAccesses =
			{
				{ R, MeshPaintComponent::Type() },
			}
		},
	};

	SubscribeToEntities(subscription);

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
