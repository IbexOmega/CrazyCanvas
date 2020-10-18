#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Multiplayer/MultiplayerServer.h"

class Level;

class ServerState :
	public LambdaEngine::State
{
public:
	ServerState();
	~ServerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	bool OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event);

private:
	Level* m_pLevel = nullptr;
	std::string m_ServerName;
	MultiplayerServer m_MultiplayerServer;
};
