#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Network/API/TCP/ServerTCP.h"
#include "Network/API/UDP/ServerUDP.h"

#include "Network/API/Discovery/NetworkDiscoveryHost.h"

#include <set>

namespace LambdaEngine
{
	class IClientTCPHandler;
	class IClientUDP;
}

class Server : 
	public LambdaEngine::Game,
	public LambdaEngine::IKeyboardHandler,
	public LambdaEngine::IServerTCPHandler,
	public LambdaEngine::IServerUDPHandler,
	public LambdaEngine::INetworkDiscoveryHostHandler
{
public:
	Server();
	~Server();

	virtual void OnSearcherRequest(LambdaEngine::NetworkPacket* packet) override;

	virtual LambdaEngine::IClientUDPHandler* CreateClientHandlerUDP() override;
	virtual LambdaEngine::IClientTCPHandler* CreateClientHandlerTCP() override;

	virtual bool OnClientAcceptedTCP(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientConnectedTCP(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientDisconnectedTCP(LambdaEngine::ClientTCP* client) override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp dt) override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

private:
	void UpdateTitle();

private:
	LambdaEngine::ServerTCP* m_pServerTCP;
	LambdaEngine::ServerUDP* m_pServerUDP;
	std::set<LambdaEngine::IClientTCPHandler*> m_ClientTCPHandlers;
	std::set<LambdaEngine::IClientUDPHandler*> m_ClientUDPHandlers;

	LambdaEngine::NetworkDiscoveryHost m_NetworkDiscovery;
};