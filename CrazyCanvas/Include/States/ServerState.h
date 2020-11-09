#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Multiplayer/MultiplayerServer.h"
#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/PacketConfigureServer.h"

#include "EventHandlers/MeshPaintHandler.h"

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

	bool OnPacketConfigureServerReceived(const PacketReceivedEvent<PacketConfigureServer>& event);

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	bool OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event);

private:
	std::string m_ServerName;
	MultiplayerServer m_MultiplayerServer;

	MeshPaintHandler m_MeshPaintHandler;
};
