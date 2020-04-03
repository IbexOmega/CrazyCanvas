#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Network/API/ServerTCP.h"
#include "Network/API/IServerTCPHandler.h"

class Server : public LambdaEngine::Game, public LambdaEngine::IKeyboardHandler, public LambdaEngine::IServerTCPHandler
{
public:
	Server();
	~Server();

	virtual bool OnClientAccepted(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientConnected(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientDisconnected(LambdaEngine::ClientTCP* client) override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp dt) override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

private:
	void UpdateTitle();

private:
	LambdaEngine::ServerTCP* m_pServer;
};
