#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Network/API/ClientTCP.h"
#include "Network/API/IClientTCPHandler.h"

class Client : public LambdaEngine::Game, public LambdaEngine::IKeyboardHandler, public LambdaEngine::IClientTCPHandler
{
public:
	Client();
	~Client();

	virtual void OnClientConnected(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientDisconnected(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientFailedConnecting(LambdaEngine::ClientTCP* client) override;

	// Inherited via Game
	virtual void Tick() override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

private:
	LambdaEngine::ClientTCP* m_pClient;
};
