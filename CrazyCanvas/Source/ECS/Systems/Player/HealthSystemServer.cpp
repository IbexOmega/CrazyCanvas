#include "ECS/Systems/Player/HealthSystemServer.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Player/HealthComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

#include "Multiplayer/Packet/PacketHealthChanged.h"

#include "Match/MatchServer.h"

#include "MeshPaint/MeshPaintHandler.h"

#include "Lobby/PlayerManagerServer.h"

#include "Resources/ResourceManager.h"

#include "RenderStages/HealthCompute.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Rendering/RenderGraph.h"

#include "Game/PlayerIndexHelper.h"

#include <mutex>

// TEMP REMOVE
#include "Input/API/Input.h"

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

	// TEMP REMOVE
	static bool pressed = false;
	if (Input::IsKeyDown(Input::GetCurrentInputmode(), EKey::KEY_F) && !pressed)
	{
		for (Entity entity : m_HealthEntities)
			HealthCompute::QueueHealthCalculation(entity);
		pressed = true;
	}
	else if (Input::IsKeyUp(Input::GetCurrentInputmode(), EKey::KEY_F) && pressed)
	{
		pressed = false;
	}

	for (Entity entity : m_HealthEntities)
		HealthCompute::QueueHealthCalculation(entity);


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

	// TEMP REMOVE
	static bool p = false;
	if (Input::IsKeyDown(Input::GetCurrentInputmode(), EKey::KEY_G) && !p)
	{
		TArray<uint32> healths = HealthCompute::GetHealths();
		for (auto health : healths)
		{
			LOG_WARNING("Health: %u", health);
		}
		LOG_WARNING("VertexCount: %u", HealthCompute::GetVertexCount());
		p = true;
	}
	else if (Input::IsKeyUp(Input::GetCurrentInputmode(), EKey::KEY_G) && p)
	{
		p = false;
	}


	// Update health
	if (!m_HitInfoToProcess.IsEmpty() && PlayerIndexHelper::GetNumOfIndices() > 0)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<HealthComponent>*		pHealthComponents		= pECS->GetComponentArray<HealthComponent>();
		ComponentArray<PacketComponent<PacketHealthChanged>>* pHealthChangedComponents = pECS->GetComponentArray<PacketComponent<PacketHealthChanged>>();

		for (HitInfo& hitInfo : m_HitInfoToProcess)
		{
			const Entity entity				= hitInfo.Player;
			const Entity projectileOwner	= hitInfo.ProjectileOwner;

			if (PlayerIndexHelper::IsEntityValid(entity))
			{
				HealthComponent& healthComponent				= pHealthComponents->GetData(entity);
				PacketComponent<PacketHealthChanged>& packets	= pHealthChangedComponents->GetData(entity);

				constexpr float32 BIASED_MAX_HEALTH	= 0.5f; // CHANGE THIS TO CHANGE AMOUNT OF HEALTH NEEDED TO DIE
				constexpr float32 START_HEALTH_F	= float32(START_HEALTH);

				// Update health
				const uint32	paintedVerticies	= HealthCompute::GetEntityHealth(entity);
				const float32	paintedHealth		= float32(paintedVerticies) / float32(HealthCompute::GetVertexCount() * (1.0f - BIASED_MAX_HEALTH));
				const int32		oldHealth			= healthComponent.CurrentHealth;
				healthComponent.CurrentHealth		= std::max<int32>(int32(START_HEALTH_F * (1.0f - paintedHealth)), 0);

				// Still here for debugging
				LOG_INFO("HIT REGISTERED: oldHealth=%d currentHealth=%d paintedVerticies=%u, paintedHealth=%.4f", oldHealth, healthComponent.CurrentHealth, paintedVerticies, paintedHealth);

				// Check if health changed
				if (oldHealth != healthComponent.CurrentHealth)
				{
					LOG_INFO("PLAYER HEALTH: CurrentHealth=%u, paintedHealth=%.4f paintedVerticies=%u VertexCount=%u",
						healthComponent.CurrentHealth,
						paintedHealth,
						paintedVerticies,
						HealthCompute::GetVertexCount());

					bool killed = false;
					if (healthComponent.CurrentHealth <= 0)
					{
						MatchServer::KillPlayer(entity, projectileOwner, false);
						killed = true;

						LOG_INFO("PLAYER DIED");
					}

					PacketHealthChanged packet = {};
					packet.CurrentHealth = healthComponent.CurrentHealth;
					packets.SendPacket(packet);
				}
			}
		}
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
			MeshPaintHandler::ResetServer(entity);

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
			
			auto it = std::find_if(m_DeferredHitInfo.Begin(), m_DeferredHitInfo.End(), [entity](const HitInfo& hitInfo){ return hitInfo.Player == entity; });

			if (it != m_DeferredHitInfo.End())
			{
				it->ProjectileOwner = projectileEntity;
			}
			else
			{
				m_DeferredHitInfo.PushBack(
					{
						projectileHitEvent.CollisionInfo1.Entity,
						projectileComponent.Owner
					});
			}

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