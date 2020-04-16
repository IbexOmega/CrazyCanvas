#pragma once

#include "Game/Game.h"

#include "Input/API/IKeyboardHandler.h"
#include "Input/API/IMouseHandler.h"

#include "Networking/API/PacketDispatcher.h"

#include <set>

class Server : 
	public LambdaEngine::Game,
	public LambdaEngine::IKeyboardHandler
{
public:
	Server();
	~Server();

	virtual void OnPacketReceived(LambdaEngine::NetworkPacket* packet, const LambdaEngine::IPEndPoint& sender);

	void Run();
	void Terminated();

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
	LambdaEngine::PacketDispatcher m_Dispatcher;
	LambdaEngine::ISocketUDP* m_pSocketUDP;

	char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	char m_pReceiveBuffer[UINT16_MAX];
};
