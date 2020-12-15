#include "ECS/Systems/GUI/HUDSystem.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"

#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/Player.h"
#include "Game/ECS/Components/Team/TeamComponent.h"


#include "ECS/ECSCore.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Application/API/Events/EventQueue.h"

#include "Lobby/PlayerManagerClient.h"

#include "Game/GameConsole.h"

using namespace LambdaEngine;

HUDSystem::~HUDSystem()
{
	m_HUDGUI.Reset();
	m_View.Reset();

	EventQueue::UnregisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);
	EventQueue::UnregisterEventHandler<WeaponReloadFinishedEvent>(this, &HUDSystem::OnWeaponReloadFinished);
	EventQueue::UnregisterEventHandler<WeaponReloadStartedEvent>(this, &HUDSystem::OnWeaponReloadStartedEvent);
	EventQueue::UnregisterEventHandler<WeaponReloadCanceledEvent>(this, &HUDSystem::OnWeaponReloadCanceledEvent);
	EventQueue::UnregisterEventHandler<FlagPickedUpEvent>(this, &HUDSystem::OnFlagPickedUpEvent);
	EventQueue::UnregisterEventHandler<FlagDroppedEvent>(this, &HUDSystem::OnFlagDroppedEvent);
	EventQueue::UnregisterEventHandler<MatchCountdownEvent>(this, &HUDSystem::OnMatchCountdownEvent);
	EventQueue::UnregisterEventHandler<ProjectileHitEvent>(this, &HUDSystem::OnProjectileHit);
	EventQueue::UnregisterEventHandler<SpectatePlayerEvent>(this, &HUDSystem::OnSpectatePlayerEvent);
	EventQueue::UnregisterEventHandler<PlayerScoreUpdatedEvent>(this, &HUDSystem::OnPlayerScoreUpdated);
	EventQueue::UnregisterEventHandler<PlayerPingUpdatedEvent>(this, &HUDSystem::OnPlayerPingUpdated);
	EventQueue::UnregisterEventHandler<PlayerAliveUpdatedEvent>(this, &HUDSystem::OnPlayerAliveUpdated);
	EventQueue::UnregisterEventHandler<GameOverEvent>(this, &HUDSystem::OnGameOver);
	EventQueue::UnregisterEventHandler<WindowResizedEvent>(this, &HUDSystem::OnWindowResized);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketTeamScored>>(this, &HUDSystem::OnPacketTeamScored);
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
			.pSubscriber = &m_LocalPlayerEntities,
			.ComponentAccesses =
			{
				{ R, HealthComponent::Type() }, { R, RotationComponent::Type() }, { NDA, PlayerLocalComponent::Type() }
			}
		},
		{
			.pSubscriber = &m_ForeignPlayerEntities,
			.ComponentAccesses =
			{
				{ NDA, PlayerForeignComponent::Type() }, { R, TeamComponent::Type() }
			}
		},
		{
			.pSubscriber = &m_ProjectedGUIEntities,
			.ComponentAccesses =
			{
				{ R, ProjectedGUIComponent::Type() }, { R, PositionComponent::Type() }
			},
			.OnEntityAdded		= std::bind_front(&HUDSystem::OnProjectedEntityAdded, this),
			.OnEntityRemoval	= std::bind_front(&HUDSystem::RemoveProjectedEntity, this)
		},
		{
			.pSubscriber = &m_CameraEntities,
			.ComponentAccesses =
			{
				{ R, CameraComponent::Type() },
				{ R, ViewProjectionMatricesComponent::Type() }
			}
		}
	};

	systemReg.SubscriberRegistration.AdditionalAccesses =
	{
		{ R, ProjectileComponent::Type() },  { R, TeamComponent::Type() }
	};

	systemReg.Phase = 1;

	RegisterSystem(TYPE_NAME(HUDSystem), systemReg);

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	EventQueue::RegisterEventHandler<WeaponFiredEvent>(this, &HUDSystem::OnWeaponFired);
	EventQueue::RegisterEventHandler<WeaponReloadFinishedEvent>(this, &HUDSystem::OnWeaponReloadFinished);
	EventQueue::RegisterEventHandler<WeaponReloadStartedEvent>(this, &HUDSystem::OnWeaponReloadStartedEvent);
	EventQueue::RegisterEventHandler<WeaponReloadCanceledEvent>(this, &HUDSystem::OnWeaponReloadCanceledEvent);
	EventQueue::RegisterEventHandler<MatchCountdownEvent>(this, &HUDSystem::OnMatchCountdownEvent);
	EventQueue::RegisterEventHandler<ProjectileHitEvent>(this, &HUDSystem::OnProjectileHit);
	EventQueue::RegisterEventHandler<FlagPickedUpEvent>(this, &HUDSystem::OnFlagPickedUpEvent);
	EventQueue::RegisterEventHandler<FlagDroppedEvent>(this, &HUDSystem::OnFlagDroppedEvent);
	EventQueue::RegisterEventHandler<SpectatePlayerEvent>(this, &HUDSystem::OnSpectatePlayerEvent);
	EventQueue::RegisterEventHandler<PlayerScoreUpdatedEvent>(this, &HUDSystem::OnPlayerScoreUpdated);
	EventQueue::RegisterEventHandler<PlayerPingUpdatedEvent>(this, &HUDSystem::OnPlayerPingUpdated);
	EventQueue::RegisterEventHandler<PlayerAliveUpdatedEvent>(this, &HUDSystem::OnPlayerAliveUpdated);
	EventQueue::RegisterEventHandler<GameOverEvent>(this, &HUDSystem::OnGameOver);
	EventQueue::RegisterEventHandler<WindowResizedEvent>(this, &HUDSystem::OnWindowResized);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketTeamScored>>(this, &HUDSystem::OnPacketTeamScored);

	m_HUDGUI = *new HUDGUI();
	m_View = Noesis::GUI::CreateView(m_HUDGUI);

	// Add players to scoreboard
	const THashTable<uint64, Player>& players = PlayerManagerClient::GetPlayers();
	for (auto& player : players)
	{
		m_HUDGUI->GetScoreBoard()->AddPlayer(player.second);
	}

	GUIApplication::SetView(m_View);

	m_LocalTeamIndex = PlayerManagerClient::GetPlayerLocal()->GetTeam();


	ConsoleCommand hideHUDCmd;
	hideHUDCmd.Init("hide_hud", false);
	hideHUDCmd.AddArg(Arg::EType::BOOL);
	hideHUDCmd.AddDescription("Hides/Shows the the HUD.\n\t'hide_hud true'");
	GameConsole::Get().BindCommand(hideHUDCmd, [&](GameConsole::CallbackInput& input)->void
	{
		m_HUDGUI->ShowHUD(!input.Arguments.GetFront().Value.Boolean);
	});
}

void HUDSystem::Tick(LambdaEngine::Timestamp deltaTime)
{

}

void HUDSystem::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<HealthComponent>* pHealthComponents = pECS->GetComponentArray<HealthComponent>();
	const ComponentArray<ViewProjectionMatricesComponent>* pViewProjMats = pECS->GetComponentArray<ViewProjectionMatricesComponent>();
	const ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

	for (Entity player : m_LocalPlayerEntities)
	{
		const HealthComponent& healthComponent = pHealthComponents->GetConstData(player);

		m_HUDGUI->UpdateHealth(healthComponent.CurrentHealth);
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
		for (auto& pair : m_DamageTakenEventsToProcess)
		{
			const ComponentArray<RotationComponent>* pPlayerRotationComp = pECS->GetComponentArray<RotationComponent>();
			const RotationComponent& playerRotationComp = pPlayerRotationComp->GetConstData(pair.first.CollisionInfo1.Entity);

			bool isFriendly = false;

			if (pair.second == m_LocalTeamIndex)
				isFriendly = true;

			m_HUDGUI->DisplayDamageTakenIndicator(GetForward(glm::normalize(playerRotationComp.Quaternion)), pair.first.CollisionInfo1.Normal, isFriendly);
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

	const ComponentArray<ProjectedGUIComponent>* pProjectedGUIComponents = pECS->GetComponentArray<ProjectedGUIComponent>();
	for (Entity camera : m_CameraEntities)
	{
		const ViewProjectionMatricesComponent& viewProjMat = pViewProjMats->GetConstData(camera);

		for (Entity entity : m_ProjectedGUIEntities)
		{
			const PositionComponent& worldPosition = pPositionComponents->GetConstData(entity);

			const glm::mat4 viewProj = viewProjMat.Projection * viewProjMat.View;

			const IndicatorTypeGUI indicatorType = pProjectedGUIComponents->GetConstData(entity).GUIType;
			m_HUDGUI->ProjectGUIIndicator(viewProj, worldPosition.Position, entity, indicatorType);
		}
	}

	static bool activeButtonChanged = false;
	if (InputActionSystem::IsActiveGlobal(EAction::ACTION_GENERAL_SCOREBOARD) && !activeButtonChanged)
	{
		m_HUDGUI->GetScoreBoard()->DisplayScoreboardMenu(true);
		activeButtonChanged = true;
	}
	else if (!InputActionSystem::IsActiveGlobal(EAction::ACTION_GENERAL_SCOREBOARD) && activeButtonChanged)
	{
		m_HUDGUI->GetScoreBoard()->DisplayScoreboardMenu(false);
		activeButtonChanged = false;
	}

	m_HUDGUI->FixedTick(delta);
}

bool HUDSystem::OnWeaponFired(const WeaponFiredEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

		if (pPlayerLocalComponents)
		{
			for (Entity weapon : m_WeaponEntities)
			{
				const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(weapon);

				if (weaponComponent.WeaponOwner == event.WeaponOwnerEntity && pPlayerLocalComponents->HasComponent(event.WeaponOwnerEntity))
				{
					m_HUDGUI->UpdateAmmo(weaponComponent.WeaponTypeAmmo, event.AmmoType);
				}
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
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

		if (pPlayerLocalComponents)
		{
			for (Entity playerWeapon : m_WeaponEntities)
			{
				const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

				if (event.WeaponOwnerEntity == weaponComponent.WeaponOwner && pPlayerLocalComponents->HasComponent(event.WeaponOwnerEntity))
				{
					m_HUDGUI->Reload(weaponComponent.WeaponTypeAmmo, false);
				}
			}
		}
	}
	return false;
}

bool HUDSystem::OnWeaponReloadStartedEvent(const WeaponReloadStartedEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

		if (pPlayerLocalComponents)
		{
			for (Entity playerWeapon : m_WeaponEntities)
			{
				const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

				if (event.WeaponOwnerEntity == weaponComponent.WeaponOwner && pPlayerLocalComponents->HasComponent(event.WeaponOwnerEntity))
				{
					m_HUDGUI->Reload(weaponComponent.WeaponTypeAmmo, true);
					PromptMessage("Reloading", true);
				}
			}
		}
	}
	return false;
}

bool HUDSystem::OnWeaponReloadCanceledEvent(const WeaponReloadCanceledEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
		const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();

		if (pPlayerLocalComponents)
		{
			for (Entity playerWeapon : m_WeaponEntities)
			{
				const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

				if (event.WeaponOwnerEntity == weaponComponent.WeaponOwner && pPlayerLocalComponents->HasComponent(event.WeaponOwnerEntity))
				{
					m_HUDGUI->AbortReload();
				}
			}
		}
	}

	return false;
}

bool HUDSystem::OnFlagPickedUpEvent(const FlagPickedUpEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		for (Entity player : m_LocalPlayerEntities)
		{
			if (event.PlayerEntity == player)
			{
				m_HUDGUI->DisplayCarryFlagIndicator(event.FlagEntity, true);
			}
		}
	}

	return false;
}

bool HUDSystem::OnFlagDroppedEvent(const FlagDroppedEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		for (Entity player : m_LocalPlayerEntities)
		{
			if (event.PlayerEntity == player)
			{
				m_HUDGUI->DisplayCarryFlagIndicator(event.FlagEntity, false);
			}
		}
	}

	return false;
}

bool HUDSystem::OnPlayerScoreUpdated(const PlayerScoreUpdatedEvent& event)
{
	m_HUDGUI->GetScoreBoard()->UpdateAllPlayerProperties(*event.pPlayer);

	return false;
}

bool HUDSystem::OnPlayerPingUpdated(const PlayerPingUpdatedEvent& event)
{
	m_HUDGUI->GetScoreBoard()->UpdatePlayerProperty(
		*event.pPlayer,
		EPlayerProperty::PLAYER_PROPERTY_PING,
		std::to_string(event.pPlayer->GetPing()));
	return false;
}

bool HUDSystem::OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event)
{
	const Player* pPlayer = event.pPlayer;
	m_HUDGUI->GetScoreBoard()->UpdatePlayerAliveStatus(*pPlayer);

	if (pPlayer->IsDead())
	{
		if(event.pPlayerKiller)
			m_HUDGUI->UpdateKillFeed(event.pPlayer->GetName(), event.pPlayerKiller->GetName(), event.pPlayer->GetTeam());
		else
			m_HUDGUI->UpdateKillFeed(event.pPlayer->GetName(), "Server", event.pPlayer->GetTeam());
	}

	if (pPlayer == PlayerManagerClient::GetPlayerLocal())
	{
		if (pPlayer->IsDead())
		{

			ECSCore* pECS = ECSCore::GetInstance();
			const ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();

			for (Entity playerWeapon : m_WeaponEntities)
			{
				const WeaponComponent& weaponComponent = pWeaponComponents->GetConstData(playerWeapon);

				if (pPlayer->GetEntity() == weaponComponent.WeaponOwner && m_HUDGUI)
				{
					m_HUDGUI->Reload(weaponComponent.WeaponTypeAmmo, false);
				}
			}

			m_HUDGUI->ShowHUD(false);

			if (event.pPlayerKiller)
			{
				String promptText = "You Were Killed By " + event.pPlayerKiller->GetName();
				PromptMessage(promptText, false);
			}
			else
			{
				String promptText = "You Were Killed By The Server";
				PromptMessage(promptText, false);
			}
		}
		else
			m_HUDGUI->ShowHUD(true);
	}

	return false;
}

bool HUDSystem::OnMatchCountdownEvent(const MatchCountdownEvent& event)
{
	m_HUDGUI->UpdateCountdown(event.CountDownTime);

	return false;
}

bool HUDSystem::OnPacketTeamScored(const PacketReceivedEvent<PacketTeamScored>& event)
{
	String promptText = " ";

	const PacketTeamScored& packet = event.Packet;
	const Player* pFlagCapturer = PlayerManagerClient::GetPlayer(packet.PlayerUID);

	if (pFlagCapturer)
		promptText = pFlagCapturer->GetName() + " Captured The Flag!";
	else
		promptText = "Team " + std::to_string(packet.TeamIndex) + " Scored!";

	PromptMessage(promptText, false, packet.TeamIndex);

	return false;
}

void HUDSystem::OnProjectedEntityAdded(LambdaEngine::Entity projectedEntity)
{
	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<ProjectedGUIComponent>* pProjectedGUIComponents = pECS->GetComponentArray<ProjectedGUIComponent>();
	const ProjectedGUIComponent& projectedGUIComponent = pProjectedGUIComponents->GetConstData(projectedEntity);

	const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	uint8 teamIndex = 0;
	if (pTeamComponents->HasComponent(projectedEntity))
	{
		const TeamComponent& teamComponent = pTeamComponents->GetConstData(projectedEntity);
		teamIndex = teamComponent.TeamIndex;
	}

	switch (projectedGUIComponent.GUIType)
	{
	case IndicatorTypeGUI::FLAG_INDICATOR:
		m_HUDGUI->CreateProjectedFlagGUIElement(projectedEntity, m_LocalTeamIndex, teamIndex);
		break;
	case IndicatorTypeGUI::PING_INDICATOR:
		m_HUDGUI->CreateProjectedPingGUIElement(projectedEntity);
		break;
	case IndicatorTypeGUI::GRENADE_INDICATOR:
		m_HUDGUI->CreateProjectedGrenadeGUIElement(projectedEntity);
		break;
	default:
		break;
	}
}

void HUDSystem::RemoveProjectedEntity(LambdaEngine::Entity projectedEntity)
{
	m_HUDGUI->RemoveProjectedGUIElement(projectedEntity);
}

bool HUDSystem::OnProjectileHit(const ProjectileHitEvent& event)
{
	if (!MultiplayerUtils::IsServer())
	{
		std::scoped_lock<SpinLock> lock(m_DeferredEventsLock);

		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<PlayerLocalComponent>*	pPlayerLocalComponents = pECS->GetComponentArray<PlayerLocalComponent>();
		const ComponentArray<TeamComponent>*		pTeamComponents = pECS->GetComponentArray<TeamComponent>();
		const ComponentArray<ProjectileComponent>*	pProjectileComponents = pECS->GetComponentArray<ProjectileComponent>();

		const ProjectileComponent& projectileComponents = pProjectileComponents->GetConstData(event.CollisionInfo0.Entity);

		if (pPlayerLocalComponents)
		{
			if (pPlayerLocalComponents->HasComponent(event.CollisionInfo1.Entity))
			{
				if (pProjectileComponents->HasComponent(event.CollisionInfo0.Entity))
				{
					uint8 projectileTeamIndex = pTeamComponents->GetConstData(projectileComponents.Owner).TeamIndex;

					m_DeferredDamageTakenHitEvents.EmplaceBack(std::make_pair(event, projectileTeamIndex));
				}
			}
			else
			{
				if (m_ForeignPlayerEntities.HasElement(event.CollisionInfo1.Entity))
				{
					if (pProjectileComponents->HasComponent(event.CollisionInfo0.Entity))
					{
						if (pTeamComponents->GetConstData(event.CollisionInfo1.Entity).TeamIndex != m_LocalTeamIndex && pPlayerLocalComponents->HasComponent(projectileComponents.Owner))
						{
							m_DeferredEnemyHitEvents.EmplaceBack(true);
						}
					}
				}
			}
		}
	}

	return false;
}

bool HUDSystem::OnSpectatePlayerEvent(const SpectatePlayerEvent& event)
{
	m_HUDGUI->DisplaySpectateText(event.PlayerName, event.IsSpectating);
	return false;
}

bool HUDSystem::OnGameOver(const GameOverEvent& event)
{
	//un-lock mouse
	Input::PushInputLayer(EInputLayer::GUI);

	const THashTable<uint64, Player>& playerMap = PlayerManagerBase::GetPlayers();

	PlayerPair mostKills((int16)-1, nullptr);
	PlayerPair mostFlags((int16)-1, nullptr);
	PlayerPair mostDeaths((int16)-1, nullptr);

	for (auto& pair : playerMap)
	{
		const Player* pPlayer = &pair.second;

		int16 kills		= pPlayer->GetKills();
		int16 deaths	= pPlayer->GetDeaths();
		int16 flags		= pPlayer->GetFlagsCaptured();

		if (kills > mostKills.first || (kills == mostKills.first && mostKills.second->GetUID() < pPlayer->GetUID()))
			mostKills = std::make_pair(kills, pPlayer);

		if (deaths > mostDeaths.first || (deaths == mostDeaths.first && mostDeaths.second->GetUID() < pPlayer->GetUID()))
			mostDeaths = std::make_pair(deaths, pPlayer);

		if (flags > mostFlags.first || (flags == mostFlags.first && mostFlags.second->GetUID() < pPlayer->GetUID()))
			mostFlags = std::make_pair(flags, pPlayer);
	}

	m_HUDGUI->DisplayGameOverGrid(event.WinningTeamIndex, mostKills, mostDeaths, mostFlags);

	return false;
}

bool HUDSystem::OnWindowResized(const WindowResizedEvent& event)
{
	m_HUDGUI->SetWindowSize(event.Width, event.Height);
	return false;
}

void HUDSystem::PromptMessage(const LambdaEngine::String& promtMessage, bool isSmallPrompt, uint8 teamIndex)
{
	m_HUDGUI->DisplayPrompt(promtMessage, isSmallPrompt, teamIndex);
}

void HUDSystem::DisplayNamePlate(const LambdaEngine::String& name, bool isLookingAtTeamPlayer)
{
	m_HUDGUI->ShowNamePlate(name, isLookingAtTeamPlayer);
}
