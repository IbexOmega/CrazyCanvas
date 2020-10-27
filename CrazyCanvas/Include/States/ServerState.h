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
	ServerState(std::string serverHostID, std::string clientHostID);
	
	~ServerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;

	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	bool OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event);

private:
	int32 m_ServerHostID = -1;
	int32 m_ClientHostID = -1;
	std::string m_ServerName;
	MultiplayerServer m_MultiplayerServer;
};
