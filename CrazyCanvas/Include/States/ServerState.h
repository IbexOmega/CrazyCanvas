#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Events/PlayerEvents.h"

#include "Multiplayer/MultiplayerServer.h"
#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/PacketGameSettings.h"

class Level;

class ServerState :
	public LambdaEngine::State
{
public:
	ServerState(const std::string& clientHostID, const std::string& authenticationID);
	
	~ServerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;

	bool OnPacketGameSettingsReceived(const PacketReceivedEvent<PacketGameSettings>& event);

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	bool OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event);

	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);

private:
	std::string m_ServerName;
	MultiplayerServer m_MultiplayerServer;
};
