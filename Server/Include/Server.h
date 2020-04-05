#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Network/API/ServerTCP.h"
#include "Network/API/ServerUDP.h"
#include "Network/API/IServerTCPHandler.h"
#include "Network/API/IServerUDPHandler.h"

class Server : public LambdaEngine::Game, public LambdaEngine::IKeyboardHandler, public LambdaEngine::IServerTCPHandler, public LambdaEngine::IServerUDPHandler
{
public:
	Server();
	~Server();

	virtual void OnPacketReceivedUDP(LambdaEngine::NetworkPacket* packet, const std::string& address, uint16 port) override;
	virtual LambdaEngine::IClientTCPHandler* CreateClientHandler() override;
	virtual bool OnClientAccepted(LambdaEngine::ClientTCP2* client) override;
	virtual void OnClientConnected(LambdaEngine::ClientTCP2* client) override;
	virtual void OnClientDisconnected(LambdaEngine::ClientTCP2* client) override;

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
};
