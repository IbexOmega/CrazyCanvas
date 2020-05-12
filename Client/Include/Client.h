#pragma once

#include "Game/Game.h"

#include "Application/API/EventHandler.h"

#include "Networking/API/IPacketListener.h"
#include "Networking/API/ClientUDP.h"
#include "Networking/API/IClientUDPHandler.h"

class Client :
	public LambdaEngine::Game,
	public LambdaEngine::EventHandler,
	public LambdaEngine::IPacketListener,
	public LambdaEngine::IClientUDPHandler
{
public:
	Client();
	~Client();

	virtual void OnConnectingUDP(LambdaEngine::IClientUDP* pClient) override;
	virtual void OnConnectedUDP(LambdaEngine::IClientUDP* pClient) override;
	virtual void OnDisconnectingUDP(LambdaEngine::IClientUDP* pClient) override;
	virtual void OnDisconnectedUDP(LambdaEngine::IClientUDP* pClient) override;
	virtual void OnPacketReceivedUDP(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkPacket* pPacket) override;
	virtual void OnServerFullUDP(LambdaEngine::IClientUDP* pClient) override;


	virtual void OnPacketDelivered(LambdaEngine::NetworkPacket* pPacket) override;
	virtual void OnPacketResent(LambdaEngine::NetworkPacket* pPacket, uint8 tries) override;
	virtual void OnPacketMaxTriesReached(LambdaEngine::NetworkPacket* pPacket, uint8 tries) override;

	// Inherited via Game
	virtual void Tick(LambdaEngine::Timestamp delta)        override;
    virtual void FixedTick(LambdaEngine::Timestamp delta)   override;

	virtual void KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)     override;

private:
	LambdaEngine::ClientUDP* m_pClient;
};
