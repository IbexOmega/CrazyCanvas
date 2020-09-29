#pragma once
#include "Game/Game.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

class Server : 
	public LambdaEngine::Game,
	public LambdaEngine::INetworkDiscoveryServer
{
public:
	Server();
	~Server();

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
	virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	virtual void OnNetworkDiscoveryPreTransmit(const LambdaEngine::BinaryEncoder& encoder) override;

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);
};
