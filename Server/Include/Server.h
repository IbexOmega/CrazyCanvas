#pragma once
#include "Game/Game.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/IServerHandler.h"
#include "Networking/API/INetworkDiscoveryServer.h"

namespace LambdaEngine
{
	class ServerBase;
	class IClientRemoteHandler;
	class IClient;
}

class Server : 
	public LambdaEngine::Game,
	public LambdaEngine::IServerHandler,
	public LambdaEngine::INetworkDiscoveryServer
{
public:
	Server();
	~Server();

	virtual LambdaEngine::IClientRemoteHandler* CreateClientHandler() override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
	virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	virtual void OnNetworkDiscoveryPreTransmit(const LambdaEngine::BinaryEncoder& encoder) override;

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

private:
	void UpdateTitle();

private:
	LambdaEngine::ServerBase* m_pServer;
};
