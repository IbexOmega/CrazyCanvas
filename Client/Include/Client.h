#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Network/API/PlatformNetworkUtils.h"

#include "Network/API/Discovery/NetworkDiscoverySearcher.h"

class Client :
	public LambdaEngine::Game,
	public LambdaEngine::IKeyboardHandler,
	public LambdaEngine::IClientTCPHandler,
	public LambdaEngine::IClientUDPHandler,
	public LambdaEngine::INetworkDiscoverySearcherHandler
{
public:
	Client();
	~Client();

	virtual void OnHostFound(const std::string& address, uint16 port, LambdaEngine::NetworkPacket* packet) override;

	virtual void OnClientPacketReceivedUDP(LambdaEngine::IClientUDP* client, LambdaEngine::NetworkPacket* packet) override;
	virtual void OnClientErrorUDP(LambdaEngine::IClientUDP* client) override;
	virtual void OnClientStoppedUDP(LambdaEngine::IClientUDP* client) override;

	virtual void OnClientConnectedTCP(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientDisconnectedTCP(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientFailedConnectingTCP(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientPacketReceivedTCP(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet) override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp dt) override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

private:
	LambdaEngine::IClientTCP* m_pClientTCP;
	LambdaEngine::IClientUDP* m_pClientUDP;

	LambdaEngine::NetworkDiscoverySearcher m_NetworkDiscovery;
};
