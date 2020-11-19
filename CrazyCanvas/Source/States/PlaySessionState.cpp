#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Input/API/Input.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "World/LevelManager.h"
#include "World/LevelObjectCreator.h"

#include "Match/Match.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

#include "Rendering/EntityMaskManager.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/SingleplayerInitializer.h"

#include "Lobby/PlayerManagerClient.h"

#include "Game/StateManager.h"
#include "States/MainMenuState.h"

#include "GUI/GUIHelpers.h"

#include "Resources/ResourceCatalog.h"

using namespace LambdaEngine;

PlaySessionState::PlaySessionState(const PacketGameSettings& gameSettings, bool singlePlayer) :
	m_Singleplayer(singlePlayer),
	m_MultiplayerClient(), 
	m_GameSettings(gameSettings)
{
	if (m_Singleplayer)
	{
		SingleplayerInitializer::Init();
	}

	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &PlaySessionState::OnClientDisconnected);
}

PlaySessionState::~PlaySessionState()
{
	if (m_Singleplayer)
	{
		SingleplayerInitializer::Release();
	}

	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(this, &PlaySessionState::OnClientDisconnected);

	Match::Release();
	PlayerManagerClient::Reset();
}

void PlaySessionState::Init()
{
	EnablePlaySessionsRenderstages();
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->Pause();

	// Initialize event listeners
	m_AudioEffectHandler.Init();
	m_MeshPaintHandler.Init();
	m_MultiplayerClient.InitInternal();

	// Load Match
	{
		const TArray<SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

		MatchDescription matchDescription =
		{
			.LevelHash	= levelHashes[m_GameSettings.MapID],
			.GameMode	= m_GameSettings.GameMode,
			.MaxScore	= m_GameSettings.FlagsToWin,
		};
		Match::CreateMatch(&matchDescription);
	}

	CommonApplication::Get()->SetMouseVisibility(false);

	if (m_Singleplayer)
	{
		SingleplayerInitializer::Setup();
	}
	else
	{
		//Called to tell the server we are ready to start the match
		PlayerManagerClient::SetLocalPlayerStateLoading();
	}

	m_HUDSystem.Init();
}

void PlaySessionState::Tick(Timestamp delta)
{
	m_MultiplayerClient.TickMainThreadInternal(delta);
}

void PlaySessionState::FixedTick(Timestamp delta)
{
	m_HUDSystem.FixedTick(delta);
	m_MultiplayerClient.FixedTickMainThreadInternal(delta);
}

bool PlaySessionState::OnClientDisconnected(const ClientDisconnectedEvent& event)
{
	const String& reason = event.Reason;

	LOG_WARNING("PlaySessionState::OnClientDisconnected(Reason: %s)", reason.c_str());

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);

	return false;
}
