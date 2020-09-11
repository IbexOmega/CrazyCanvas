#pragma once

#include "Game/Game.h"

#include "Application/API/EventHandler.h"


#include "Networking/API/IServerHandler.h"
#include "Networking/API/IClient.h"
#include "Networking/API/ServerBase.h"
#include "Networking/API/IClientRemoteHandler.h"

class Server : 
	public LambdaEngine::Game,
	public LambdaEngine::EventHandler,
	public LambdaEngine::IServerHandler
{
public:
	Server();
	~Server();

	virtual void OnClientConnected(LambdaEngine::IClient* pClient) override;
	virtual LambdaEngine::IClientRemoteHandler* CreateClientHandler() override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	virtual void OnKeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)     override;

private:
	void UpdateTitle();

private:
	LambdaEngine::ServerBase* m_pServer;
};
