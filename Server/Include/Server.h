#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Networking/API/ServerUDP.h"
#include "Networking/API/IServerUDPHandler.h"
#include "Networking/API/IClientUDP.h"

#include "Networking/API/IClientUDPHandler.h"

#include <set>

class Server : 
	public LambdaEngine::Game,
	public LambdaEngine::IKeyboardHandler,
	public LambdaEngine::IServerUDPHandler
{
public:
	Server();
	~Server();

	virtual void OnClientConnected(LambdaEngine::IClientUDP* pClient) override;
	virtual LambdaEngine::IClientUDPHandler* CreateClientUDPHandler() override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	// Inherited via IKeyboardHandler
	virtual void OnKeyDown(LambdaEngine::EKey key)      override;
	virtual void OnKeyHeldDown(LambdaEngine::EKey key)  override;
	virtual void OnKeyUp(LambdaEngine::EKey key)        override;

private:
	void UpdateTitle();

private:
	LambdaEngine::ServerUDP* m_pServer;
};
