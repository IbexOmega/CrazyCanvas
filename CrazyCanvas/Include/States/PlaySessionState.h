#pragma once
#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Systems/Misc/DestructionSystem.h"

#include "Game/State.h"

#include "ECS/Systems/GUI/HUDSystem.h"
#include "ECS/Systems/Camera/SpectateCameraSystem.h"

#include "Application/API/Events/NetworkEvents.h"

#include "EventHandlers/AudioEffectHandler.h"
#include "EventHandlers/MeshPaintHandler.h"

#include "Multiplayer/MultiplayerClient.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Multiplayer/Packet/PacketGameSettings.h"

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

	const PacketGameSettings& GetGameSettings() const;

public:
	static PlaySessionState* GetInstance();

private:
	bool m_Singleplayer;

	PacketGameSettings m_GameSettings;

	/* Systems */
	HUDSystem m_HUDSystem;
	SpectateCameraSystem m_CamSystem;
	MultiplayerClient m_MultiplayerClient;
	DestructionSystem m_DestructionSystem;

	/* Event handlers */
	AudioEffectHandler m_AudioEffectHandler;
	MeshPaintHandler m_MeshPaintHandler;

private:
	static PlaySessionState* s_pInstance;
};
