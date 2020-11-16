#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/MatchServer.h"

#include "EventHandlers/MeshPaintHandler.h"

#include "Lobby/PlayerManagerServer.h"

#include <mutex>

/*
* HealthSystemServer
*/

HealthSystemServer::~HealthSystemServer()
{
	using namespace LambdaEngine;

	EventQueue::UnregisterEventHandler<ProjectileHitEvent>(this, &HealthSystemServer::OnProjectileHit);
}

void HealthSystemServer::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(deltaTime);

	// More threadsafe
	{
		std::scoped_lock<SpinLock> lock(m_DeferredHitInfoLock);
		if (!m_DeferredHitInfo.IsEmpty())
		{
			m_HitInfoToProcess = m_DeferredHitInfo;
			m_DeferredHitInfo.Clear();
		}
	}

	// More threadsafe
	{
		std::scoped_lock<SpinLock> lock(m_DeferredResetsLock);
		if (!m_DeferredResets.IsEmpty())
		{
			m_ResetsToProcess = m_DeferredResets;
			m_DeferredResets.Clear();
		}
	}

	// Update health
	if (!m_HitInfoToProcess.IsEmpty())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<HealthComponent>*		pHealthComponents		= pECS->GetComponentArray<HealthComponent>();
		ComponentArray<MeshPaintComponent>*		pMeshPaintComponents	= pECS->GetComponentArray<MeshPaintComponent>();
		ComponentArray<PacketComponent<PacketHealthChanged>>* pHealthChangedComponents = pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

		for (HitInfo& hitInfo : m_HitInfoToProcess)
		{
			const Entity entity				= hitInfo.Player;
			const Entity projectileOwner	= hitInfo.ProjectileOwner;

			HealthComponent& healthComponent				= pHealthComponents->GetData(entity);
			PacketComponent<PacketHealthChanged>& packets	= pHealthChangedComponents->GetData(entity);
			MeshPaintComponent& meshPaintComponent			= pMeshPaintComponents->GetData(entity);

			// Check painted pixels
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
					const byte mask = pPaintMask[(y * SIZE_X) + x].Red;
					const bool isPainted = IS_MASK_PAINTED(mask);
					if (isPainted)
					{
						paintedPixels++;
					}
				}
			}

			pBuffer->Unmap();

			constexpr float32 BIASED_MAX_HEALTH	= 0.15f;
			constexpr float32 MAX_PIXELS		= float32(SIZE_Y * SIZE_X * BIASED_MAX_HEALTH);
			constexpr float32 START_HEALTH_F	= float32(START_HEALTH);

			// Update health
			const float32	paintedHealth	= float32(paintedPixels) / float32(MAX_PIXELS);
			const int32		oldHealth		= healthComponent.CurrentHealth;
			healthComponent.CurrentHealth	= std::max<int32>(int32(START_HEALTH_F * (1.0f - paintedHealth)), 0);

			LOG_INFO("HIT REGISTERED: oldHealth=%d currentHealth=%d", oldHealth, healthComponent.CurrentHealth);

			// Check if health changed
			if (oldHealth != healthComponent.CurrentHealth)
			{
				LOG_INFO("PLAYER HEALTH: CurrentHealth=%u, paintedHealth=%.4f paintedPixels=%u MAX_PIXELS=%.4f",
					healthComponent.CurrentHealth,
					paintedHealth,
					paintedPixels,
					MAX_PIXELS);

				bool killed = false;
				if (healthComponent.CurrentHealth <= 0)
				{
					MatchServer::KillPlayer(entity, projectileOwner);
					killed = true;

					LOG_INFO("PLAYER DIED");
				}

				PacketHealthChanged packet = {};
				packet.CurrentHealth = healthComponent.CurrentHealth;
				packets.SendPacket(packet);
			}
		}

		m_HitInfoToProcess.Clear();
	}

	// Reset healthcomponents
	if (!m_ResetsToProcess.IsEmpty())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<HealthComponent>*						pHealthComponents			= pECS->GetComponentArray<HealthComponent>();
		ComponentArray<PacketComponent<PacketHealthChanged>>*	pHealthChangedComponents	= pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

		for (Entity entity : m_ResetsToProcess)
		{
			// Reset texture
			PaintMaskRenderer::ResetServer(entity);

			// Reset health and send to client
			HealthComponent& healthComponent	= pHealthComponents->GetData(entity);
			healthComponent.CurrentHealth		= START_HEALTH;

			PacketComponent<PacketHealthChanged>& packets = pHealthChangedComponents->GetData(entity);
			PacketHealthChanged packet = {};
			packet.CurrentHealth = healthComponent.CurrentHealth;
			packets.SendPacket(packet);
		}

		m_ResetsToProcess.Clear();
	}
}

bool HealthSystemServer::InitInternal()
{
	using namespace LambdaEngine;

	// Register system
	{
		SystemRegistration systemReg = {};
		HealthSystem::CreateBaseSystemRegistration(systemReg);
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(
		{
			.pSubscriber = &m_MeshPaintEntities,
			.ComponentAccesses =
			{
				{ NDA, PlayerLocalComponent::Type() },
			},
		});

		systemReg.SubscriberRegistration.AdditionalAccesses.PushBack(
			{ R, ProjectileComponent::Type() }
		);
		
		RegisterSystem(TYPE_NAME(HealthSystemServer), systemReg);
	}

	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HealthSystemServer::OnProjectileHit);
	return true;
}

bool HealthSystemServer::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_DeferredHitInfoLock);
	for (Entity entity : m_HealthEntities)
	{
		// CollisionInfo0 is the projectile
		if (projectileHitEvent.CollisionInfo1.Entity == entity)
		{
			const Entity projectileEntity = projectileHitEvent.CollisionInfo0.Entity;
			
			ECSCore* pECS = ECSCore::GetInstance();
			const ProjectileComponent& projectileComponent = pECS->GetConstComponent<ProjectileComponent>(projectileEntity);
			
			m_DeferredHitInfo.PushBack(
				{ 
					projectileHitEvent.CollisionInfo1.Entity,
					projectileComponent.Owner
				});

			break;
		}
	}

	return true;
}

void HealthSystemServer::InternalResetHealth(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_DeferredResetsLock);
	m_DeferredResets.EmplaceBack(entity);
}

void HealthSystemServer::ResetHealth(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	HealthSystemServer* pServerHealthSystem = static_cast<HealthSystemServer*>(s_Instance.Get());
	VALIDATE(pServerHealthSystem != nullptr);

	pServerHealthSystem->InternalResetHealth(entity);
}
