#pragma once

#include "ECS/Systems/Player/WeaponSystem.h"
#include "Game/State.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Networking/API/IPAddress.h"

class Level;

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState(LambdaEngine::IPAddress* pIPAddress);
	~PlaySessionState();

	void Init() override final;

	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;

private:
	Level* m_pLevel = nullptr;

	LambdaEngine::IPAddress* m_pIPAddress;

	WeaponSystem m_WeaponSystem;
};
