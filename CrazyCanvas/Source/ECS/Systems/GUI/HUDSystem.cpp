#include "ECS/Systems/GUI/HUDSystem.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"

#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/Components/Match/FlagComponent.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Application/API/Events/EventQueue.h"

#include "Lobby/PlayerManagerClient.h"


using namespace LambdaEngine;

HUDSystem::~HUDSystem()
{
	m_HUDGUI.Reset();
	m_View.Reset();

	EventQueue::UnregisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);
	EventQueue::UnregisterEventHandler<WeaponReloadFinishedEvent>(this, &HUDSystem::OnWeaponReloadFinished);
	EventQueue::UnregisterEventHandler<MatchCountdownEvent>(this, &HUDSystem::OnMatchCountdownEvent);
	EventQueue::UnregisterEventHandler<ProjectileHitEvent>(this, &HUDSystem::OnProjectileHit);
	EventQueue::UnregisterEventHandler<PlayerScoreUpdatedEvent>(this, &HUDSystem::OnPlayerScoreUpdated);
	EventQueue::UnregisterEventHandler<PlayerPingUpdatedEvent>(this, &HUDSystem::OnPlayerPingUpdated);
	EventQueue::UnregisterEventHandler<PlayerAliveUpdatedEvent>(this, &HUDSystem::OnPlayerAliveUpdated);
	EventQueue::UnregisterEventHandler<GameOverEvent>(this, &HUDSystem::OnGameOver);
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
				{ R, HealthComponent::Type() }, { R, CameraComponent::Type() }, { R, RotationComponent::Type() }, { NDA, PlayerLocalComponent::Type() }
			}
		},
		{
			.pSubscriber = &m_ForeignPlayerEntities,
			.ComponentAccesses =
			{
				{ NDA,	PlayerForeignComponent::Type() }, { R,	TeamComponent::Type() }
			}
		},
		{
			.pSubscriber = &m_FlagEntities,
			.ComponentAccesses =
			{
				{ R,	FlagComponent::Type() }
			}
		}
	};

	systemReg.SubscriberRegistration.AdditionalAccesses =
	{
		{ R, ProjectileComponent::Type() }
	};

	systemReg.Phase = 1;

	RegisterSystem(TYPE_NAME(HUDSystem), systemReg);

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	EventQueue::RegisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);
	EventQueue::RegisterEventHandler<WeaponReloadFinishedEvent>(this, &HUDSystem::OnWeaponReloadFinished);
	EventQueue::RegisterEventHandler<MatchCountdownEvent>(this, &HUDSystem::OnMatchCountdownEvent);
	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HUDSystem::OnProjectileHit);
	EventQueue::RegisterEventHandler<PlayerScoreUpdatedEvent>(this, &HUDSystem::OnPlayerScoreUpdated);
	EventQueue::RegisterEventHandler<PlayerPingUpdatedEvent>(this, &HUDSystem::OnPlayerPingUpdated);
	EventQueue::RegisterEventHandler<PlayerAliveUpdatedEvent>(this, &HUDSystem::OnPlayerAliveUpdated);
	EventQueue::RegisterEventHandler<GameOverEvent>(this, &HUDSystem::OnGameOver);

	m_HUDGUI = *new HUDGUI();
	m_View = Noesis::GUI::CreateView(m_HUDGUI);

	// Add players to scoreboard
	const THashTable<uint64, Player>& players = PlayerManagerClient::GetPlayers();
	for (auto& player : players)
	{
		m_HUDGUI->AddPlayer(player.second);
	}

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
	const ComponentArray<CameraComponent>* pCameraComponents = pECS->GetComponentArray<CameraComponent>();
	const ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

	for (Entity player : m_PlayerEntities)
	{
		const HealthComponent& healthComponent = pHealthComponents->GetConstData(player);
		m_HUDGUI->UpdateScore();
		m_HUDGUI->UpdateHealth(healthComponent.CurrentHealth);

		if (m_LocalTeamIndex == UINT32_MAX)
		{
			const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();
			m_LocalTeamIndex = pTeamComponents->GetConstData(player).TeamIndex;
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

		const CameraComponent& cameraComponent = pCameraComponents->GetConstData(player);

		for (Entity flag : m_FlagEntities)
		{
			const PositionComponent& cameraWorldPosition = pPositionComponents->GetConstData(flag);
			
			m_HUDGUI->UpdateFlagIndicator(delta, cameraComponent.ViewInv * cameraComponent.ProjectionInv, cameraWorldPosition.Position);
		}
	}

	static bool activeButtonChanged = false;
	if (InputActionSystem::IsActive(EAction::ACTION_GENERAL_SCOREBOARD) && !activeButtonChanged)
	{
		m_HUDGUI->DisplayScoreboardMenu(true);
		activeButtonChanged = true;
	}
	else if (!InputActionSystem::IsActive(EAction::ACTION_GENERAL_SCOREBOARD) && activeButtonChanged)
	{
		m_HUDGUI->DisplayScoreboardMenu(false);
		activeButtonChanged = false;
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

bool HUDSystem::OnPlayerScoreUpdated(const PlayerScoreUpdatedEvent& event)
{
	m_HUDGUI->UpdateAllPlayerProperties(*event.pPlayer);
	return false;
}

bool HUDSystem::OnPlayerPingUpdated(const PlayerPingUpdatedEvent& event)
{
	m_HUDGUI->UpdatePlayerProperty(
		event.pPlayer->GetUID(),
		EPlayerProperty::PLAYER_PROPERTY_PING,
		std::to_string(event.pPlayer->GetPing()));
	return false;
}

bool HUDSystem::OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event)
{
	const Player* pPlayer = event.pPlayer;

	m_HUDGUI->UpdatePlayerAliveStatus(pPlayer->GetUID(), !pPlayer->IsDead());
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
			const ComponentArray<ProjectileComponent>* pProjectileComponents = pECS->GetComponentArray<ProjectileComponent>();

			if (m_ForeignPlayerEntities.HasElement(event.CollisionInfo1.Entity))
			{
				if (pProjectileComponents->HasComponent(event.CollisionInfo0.Entity)) 
				{
					const ProjectileComponent& projectileComponents = pProjectileComponents->GetConstData(event.CollisionInfo0.Entity);

					if (pTeamComponents->GetConstData(event.CollisionInfo1.Entity).TeamIndex != m_LocalTeamIndex && pPlayerLocalComponents->HasComponent(projectileComponents.Owner))
					{
						m_DeferredEnemyHitEvents.EmplaceBack(true);
					}
				}
			}
		}
	}

	return false;
}

bool HUDSystem::OnGameOver(const GameOverEvent& event)
{
	//un-lock mouse
	Input::PushInputMode(EInputLayer::GUI);

	const THashTable<uint64, Player>& playerMap = PlayerManagerBase::GetPlayers();

	PlayerPair mostKills((uint8)0, nullptr);
	PlayerPair mostFlags((uint8)0, nullptr);
	PlayerPair mostDeaths((uint8)0, nullptr);

	for (auto& pair : playerMap)
	{
		const Player* pPlayer = &pair.second;

		uint8 kills = pPlayer->GetKills();
		uint8 deaths = pPlayer->GetDeaths();
		uint8 flags = pPlayer->GetFlagsCaptured();
		
		if (kills >= mostKills.first)
			mostKills = std::make_pair(kills, pPlayer);

		if (deaths >= mostDeaths.first)
			mostDeaths = std::make_pair(deaths, pPlayer);

		if (flags >= mostFlags.first)
			mostFlags = std::make_pair(flags, pPlayer);
	}

	m_HUDGUI->DisplayGameOverGrid(event.WinningTeamIndex, mostKills, mostFlags, mostDeaths);

	return false;
}
