#include "Game/Multiplayer/Server/ClientRemoteSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/NetworkEvents.h"

namespace LambdaEngine
{
	ClientRemoteSystem::ClientRemoteSystem() :
		m_pClient(nullptr)
	{
		
	}

	ClientRemoteSystem::~ClientRemoteSystem()
	{

	}

	void ClientRemoteSystem::OnConnecting(IClient* pClient)
	{
		m_pClient = (ClientRemoteBase*)pClient;

		ClientConnectingEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	void ClientRemoteSystem::OnConnected(IClient* pClient)
	{
		ClientConnectedEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	void ClientRemoteSystem::OnDisconnecting(IClient* pClient)
	{
		ClientDisconnectingEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	void ClientRemoteSystem::OnDisconnected(IClient* pClient)
	{
		ClientDisconnectedEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	void ClientRemoteSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		NetworkSegmentReceivedEvent event(pClient, pPacket);
		EventQueue::SendEventImmediate(event);
	}

	void ClientRemoteSystem::OnClientReleased(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
		delete this;
	}
}