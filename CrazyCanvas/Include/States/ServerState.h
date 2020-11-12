#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Events/PlayerEvents.h"
#include "Events/PacketEvents.h"

#include "Multiplayer/MultiplayerServer.h"
#include "Multiplayer/Packet/PacketGameSettings.h"

#include "EventHandlers/MeshPaintHandler.h"

class Level;

class ServerState :
	public LambdaEngine::State
{
public:
	ServerState(const std::string& clientHostID);
	
	~ServerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;

	bool OnPacketGameSettingsReceived(const PacketReceivedEvent<PacketGameSettings>& event);

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	bool OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event);
	bool OnPlayerJoinedEvent(const PlayerJoinedEvent& event);

private:
	int32 m_ClientHostID;
	PacketGameSettings m_GameSettings;
	std::string m_MapName;

	MultiplayerServer m_MultiplayerServer;
	MeshPaintHandler m_MeshPaintHandler;
};
