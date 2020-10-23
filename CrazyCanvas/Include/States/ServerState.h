#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

#include "Application/API/Events/NetworkEvents.h"

class Level;

class ServerState :
	public LambdaEngine::State
{
public:
	ServerState() = default;
	ServerState(std::string serverHostID, std::string clientHostID);
	
	~ServerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);

	virtual void Tick(LambdaEngine::Timestamp delta) override;

	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	bool OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event);

private:
	Level* m_pLevel = nullptr;
	int32 m_ServerHostID = -1;
	int32 m_ClientHostID = -1;
	std::string m_ServerName;
};
