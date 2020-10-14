#pragma once

#include "ECS/Systems/Player/WeaponSystem.h"
#include "Game/State.h"

#include "Application/API/Events/NetworkEvents.h"
#include "EventHandlers/MeshPaintHandler.h"

class Level;

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState(bool online);
	~PlaySessionState();

	void Init() override final;

	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;

private:
	Level* m_pLevel = nullptr;
	bool m_Online;

	/* Systems */
	WeaponSystem m_WeaponSystem;

	/* Event handlers */
	MeshPaintHandler m_MeshPaintHandler;
};
