#pragma once
#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/Systems/Match/FlagSystemBase.h"

#include "Game/State.h"

#include "ECS/Systems/GUI/HUDSystem.h"

#include "Application/API/Events/NetworkEvents.h"

#include "EventHandlers/AudioEffectHandler.h"
#include "EventHandlers/MeshPaintHandler.h"

#include "Multiplayer/MultiplayerClient.h"

class Level;

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState(bool singlePlayer = false);
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

private:
	bool m_Singleplayer;

	/* Systems */
	HUDSystem m_HUDSystem;
	MultiplayerClient m_MultiplayerClient;

	/* Event handlers */
	AudioEffectHandler m_AudioEffectHandler;
	MeshPaintHandler m_MeshPaintHandler;
};
