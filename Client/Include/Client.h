#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Network/API/ClientTCP.h"
#include "Network/API/ClientUDP.h"
#include "Network/API/IClientTCPHandler.h"
#include "Network/API/IClientUDPHandler.h"

class Client : public LambdaEngine::Game, public LambdaEngine::IKeyboardHandler, public LambdaEngine::IClientTCPHandler, public LambdaEngine::IClientUDPHandler
{
public:
	Client();
	~Client();

	virtual void OnClientPacketReceivedUDP(LambdaEngine::ClientUDP* client, LambdaEngine::NetworkPacket* packet) override;
	virtual void OnClientErrorUDP(LambdaEngine::ClientUDP* client) override;
	virtual void OnClientStoppedUDP(LambdaEngine::ClientUDP* client) override;

	virtual void OnClientConnected(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientDisconnected(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientFailedConnecting(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientPacketReceived(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet) override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp dt) override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

private:
	LambdaEngine::ClientTCP* m_pClientTCP;
	LambdaEngine::ClientUDP* m_pClientUDP;
};
