#include "Application/API/CommonApplication.h"
#include "ECS/Systems/GUI/HUDSystem.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Team/TeamComponent.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"


#include "Application/API/Events/EventQueue.h"
#include "..\..\..\..\Include\ECS\Systems\GUI\HUDSystem.h"


using namespace LambdaEngine;

HUDSystem::~HUDSystem()
{
	m_HUDGUI.Reset();
	m_View.Reset();

	EventQueue::UnregisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);
	EventQueue::UnregisterEventHandler<WeaponReloadFinishedEvent>(this, &HUDSystem::OnWeaponReloadFinished);
	EventQueue::UnregisterEventHandler<MatchCountdownEvent>(this, &HUDSystem::OnMatchCountdownEvent);
	EventQueue::UnregisterEventHandler<ProjectileHitEvent>(this, &HUDSystem::OnProjectileHit);
}

void HUDSystem::Init()
{

	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_WeaponEntities,
			.ComponentAccesses =
			{
				{ R, WeaponComponent::Type() },
			}
		},
		{
			.pSubscriber = &m_PlayerEntities,
			.ComponentAccesses =
			{
				{ R, HealthComponent::Type() }, { R, RotationComponent::Type() }, { NDA, PlayerLocalComponent::Type() }
			}
		},
		{
			.pSubscriber = &m_ForeignPlayerEntities,
			.ComponentAccesses =
			{
				{ NDA,	PlayerForeignComponent::Type() }, { R,	TeamComponent::Type() }
			}
		}
	};
	systemReg.Phase = 1;

	RegisterSystem(TYPE_NAME(HUDSystem), systemReg);

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	EventQueue::RegisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);
	EventQueue::RegisterEventHandler<WeaponReloadFinishedEvent>(this, &HUDSystem::OnWeaponReloadFinished);
    EventQueue::RegisterEventHandler<MatchCountdownEvent>(this, &HUDSystem::OnMatchCountdownEvent);
	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HUDSystem::OnProjectileHit);

	m_HUDGUI = *new HUDGUI();
	m_View = Noesis::GUI::CreateView(m_HUDGUI);

	GUIApplication::SetView(m_View);
}

void HUDSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
}

void HUDSystem::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();

	for (Entity players : m_PlayerEntities)
	{
		const HealthComponent& healthComponent = pHealthComponents->GetConstData(players);
		m_HUDGUI->UpdateScore();
		m_HUDGUI->UpdateHealth(healthComponent.CurrentHealth);

		if (m_LocalTeamIndex == UINT32_MAX)
		{
			const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();
			m_LocalTeamIndex = pTeamComponents->GetConstData(players).TeamIndex;
		}
	}

	{
		std::scoped_lock<SpinLock> lock(m_DeferredEventsLock);
		if (!m_DeferredDamageTakenHitEvents.IsEmpty())
		{
			m_DamageTakenEventsToProcess = m_DeferredDamageTakenHitEvents;
			m_DeferredDamageTakenHitEvents.Clear();
		}

		if (!m_DeferredEnemyHitEvents.IsEmpty())
		{
			m_EnemyHitEventsToProcess = m_DeferredEnemyHitEvents;
			m_DeferredEnemyHitEvents.Clear();
		}
	}

	if (!m_DamageTakenEventsToProcess.IsEmpty())
	{
		for (ProjectileHitEvent& event : m_DamageTakenEventsToProcess)
		{
			const ComponentArray<RotationComponent>* pPlayerRotationComp = pECS->GetComponentArray<RotationComponent>();
			const RotationComponent& playerRotationComp = pPlayerRotationComp->GetConstData(event.CollisionInfo1.Entity);

			m_HUDGUI->DisplayDamageTakenIndicator(GetForward(glm::normalize(playerRotationComp.Quaternion)), event.CollisionInfo1.Normal);
		}

		m_DamageTakenEventsToProcess.Clear();
	}

	if (!m_EnemyHitEventsToProcess.IsEmpty())
	{
		for (uint32 i = 0; i < m_EnemyHitEventsToProcess.GetSize(); i++)
		{
			m_HUDGUI->DisplayHitIndicator();
		}

		m_EnemyHitEventsToProcess.Clear();
	}

}

bool HUDSystem::OnWeaponFired(const WeaponFiredEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

		for (Entity playerWeapon : m_WeaponEntities)
		{
			const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

			if (pPlayerLocalComponents->HasComponent(weaponComponent.WeaponOwner) && m_HUDGUI)
			{
				m_HUDGUI->UpdateAmmo(weaponComponent.WeaponTypeAmmo, event.AmmoType);
			}
		}
	}
	return false;
}

bool HUDSystem::OnWeaponReloadFinished(const WeaponReloadFinishedEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();

		for (Entity playerWeapon : m_WeaponEntities)
		{
			const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

			if (event.WeaponOwnerEntity == weaponComponent.WeaponOwner && m_HUDGUI)
			{
				m_HUDGUI->UpdateAmmo(weaponComponent.WeaponTypeAmmo, EAmmoType::AMMO_TYPE_PAINT);
				m_HUDGUI->UpdateAmmo(weaponComponent.WeaponTypeAmmo, EAmmoType::AMMO_TYPE_WATER);
			}
		}
	}
	return false;
}

bool HUDSystem::OnMatchCountdownEvent(const MatchCountdownEvent& event)
{
	m_HUDGUI->UpdateCountdown(event.CountDownTime);

	return false;
}

bool HUDSystem::OnProjectileHit(const ProjectileHitEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		std::scoped_lock<SpinLock> lock(m_DeferredEventsLock);

		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();
		
		if (pPlayerLocalComponents->HasComponent(event.CollisionInfo1.Entity))
		{
			m_DeferredDamageTakenHitEvents.EmplaceBack(event);
		}
		else
		{
			const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

			if (m_ForeignPlayerEntities.HasElement(event.CollisionInfo1.Entity))
			{
				if (pTeamComponents->GetConstData(event.CollisionInfo1.Entity).TeamIndex != m_LocalTeamIndex)
				{
					m_DeferredEnemyHitEvents.EmplaceBack(true);
				}
			}
		}
	}

	return false;
}
