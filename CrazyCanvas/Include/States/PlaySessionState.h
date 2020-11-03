#pragma once
#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystem.h"
#include "ECS/Systems/Match/FlagSystemBase.h"

#include "Game/State.h"

#include "ECS/Systems/GUI/HUDSystem.h"

#include "Application/API/Events/NetworkEvents.h"

#include "EventHandlers/AudioEffectHandler.h"
#include "EventHandlers/MeshPaintHandler.h"

#include "Networking/API/IPAddress.h"

#include "Multiplayer/MultiplayerClient.h"

class Level;

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState(bool singlePlayer, const LambdaEngine::IPEndPoint& endPoint);
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
	LambdaEngine::IPEndPoint m_EndPoint;

	/* Systems */
	HUDSystem m_HUDSystem;
	MultiplayerClient m_MultiplayerClient;

	/* Event handlers */
	AudioEffectHandler m_AudioEffectHandler;
	MeshPaintHandler m_MeshPaintHandler;
};
