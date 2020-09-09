#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IServerHandler.h"
#include "Networking/API/BinaryEncoder.h"

#include "Networking/API/TCP/ClientTCPRemote.h"
#include "Networking/API/TCP/ISocketTCP.h"
#include "Networking/API/TCP/ServerTCP.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ServerTCP::ServerTCP(const ServerTCPDesc& desc) :
		m_Desc(desc),
		m_PacketLoss(0.0f)
	{
		
	}

	ServerTCP::~ServerTCP()
	{
		
	}

	ISocket* ServerTCP::SetupSocket()
	{
		ISocketTCP* pSocket = PlatformNetworkUtils::CreateSocketTCP();
		if (pSocket)
		{
			if (pSocket->Bind(GetEndPoint()))
			{
				if (pSocket->Listen())
				{
					LOG_INFO("[ServerTCP]: Started %s", GetEndPoint().ToString().c_str());
					return pSocket;
				}
			}
		}
		return nullptr;
	}

	void ServerTCP::RunReceiver()
	{
		while (!ShouldTerminate())
		{
			ISocketTCP* socket = ((ISocketTCP*)m_pSocket)->Accept();
			if (!socket)
				continue;

			ClientTCPRemote* pClient = CreateClient(socket);

			if (!IsAcceptingConnections())
			{
				SendServerNotAccepting(pClient);
				pClient->Release();
				continue;
			}
			else if (GetClientCount() >= m_Desc.MaxClients)
			{
				SendServerFull(pClient);
				pClient->Release();
				continue;
			}
			else
			{
				RegisterClient(pClient);
			}
		}
	}

	void ServerTCP::TransmitPacketsForClient(ClientRemoteBase* pClient)
	{
		ClientTCPRemote* client = (ClientTCPRemote*)pClient;
		client->SendPackets();
	}

	IClientRemoteHandler* ServerTCP::CreateClientHandler()
	{
		return m_Desc.Handler->CreateClientHandler();
	}

	ClientTCPRemote* ServerTCP::CreateClient(ISocketTCP* socket)
	{
		return DBG_NEW ClientTCPRemote(m_Desc.PoolSize, socket, this);
	}

	void ServerTCP::OnClientDisconnected(ClientTCPRemote* pClient, bool sendDisconnectPacket)
	{
		if(sendDisconnectPacket)
			SendDisconnect(pClient);

		UnRegisterClient(pClient);
	}

	void ServerTCP::SendDisconnect(ClientTCPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_DISCONNECT));
		client->SendPackets();
	}

	void ServerTCP::SendServerFull(ClientTCPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_SERVER_FULL));
		client->SendPackets();
	}

	void ServerTCP::SendServerNotAccepting(ClientTCPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_SERVER_NOT_ACCEPTING));
		client->SendPackets();
	}

	ServerTCP* ServerTCP::Create(const ServerTCPDesc& desc)
	{
		return DBG_NEW ServerTCP(desc);
	}
}