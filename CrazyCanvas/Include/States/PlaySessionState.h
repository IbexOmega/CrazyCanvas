#pragma once

#include "ECS/Systems/GUI/NameplateSystem.h"
#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Systems/Misc/DestructionSystem.h"
#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystem.h"

#include "Game/State.h"

#include "ECS/Systems/Camera/SpectateCameraSystem.h"
#include "ECS/Systems/GUI/HUDSystem.h"
#include "ECS/Systems/Player/GrenadeSystem.h"

#include "Application/API/Events/NetworkEvents.h"

#include "EventHandlers/AudioEffectHandler.h"
#include "EventHandlers/PingHandler.h"

#include "Multiplayer/MultiplayerClient.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Multiplayer/Packet/PacketGameSettings.h"
#include "Multiplayer/Packet/PacketMatchReady.h"

class Level;

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState(const PacketGameSettings& gameSettings, bool singlePlayer = false);
	~PlaySessionState();

	void Init() override final;

	void Resume() override final
	{
	}

	void Pause() override final
	{
	}

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;
	bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	bool OnPacketMatchReadyReceived(const PacketReceivedEvent<PacketMatchReady>& event);

	const PacketGameSettings& GetGameSettings() const;

private:
	void InternalInit();
	void TryFinishMatchLoading();

public:
	static PlaySessionState* GetInstance();

private:
	bool m_Singleplayer;
	uint8 m_DefferedTicks;
	bool m_Initiated;
	bool m_MatchReadyReceived;
	bool m_MatchLoaded;

	PacketGameSettings m_GameSettings;

	/* Systems */
	HUDSystem m_HUDSystem;
	NameplateSystem m_NameplateSystem;
	SpectateCameraSystem m_CamSystem;
	MultiplayerClient m_MultiplayerClient;
	DestructionSystem m_DestructionSystem;
	GrenadeSystem m_GrenadeSystem;

	/* Event handlers */
	AudioEffectHandler m_AudioEffectHandler;
	PingHandler m_PingHandler;

	/* Commands */
	bool m_UpdateShaders = false;

private:
	static PlaySessionState* s_pInstance;
};
