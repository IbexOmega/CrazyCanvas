#pragma once

#include "Game/Game.h"

#include "Application/API/EventHandler.h"

#include "Networking/API/ServerUDP.h"
#include "Networking/API/IServerUDPHandler.h"
#include "Networking/API/IClientUDP.h"

#include "Networking/API/IClientUDPRemoteHandler.h"

#include <set>

class Server : 
	public LambdaEngine::Game,
	public LambdaEngine::EventHandler,
	public LambdaEngine::IServerUDPHandler
{
public:
	Server();
	~Server();

	virtual void OnClientConnected(LambdaEngine::IClientUDP* pClient) override;
	virtual LambdaEngine::IClientUDPRemoteHandler* CreateClientUDPHandler() override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	virtual void KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)     override;

private:
	void UpdateTitle();

private:
	LambdaEngine::ServerUDP* m_pServer;
};
