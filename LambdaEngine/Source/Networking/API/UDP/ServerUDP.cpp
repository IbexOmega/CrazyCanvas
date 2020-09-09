#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IServerHandler.h"
#include "Networking/API/BinaryEncoder.h"

#include "Networking/API/UDP/ClientUDPRemote.h"
#include "Networking/API/UDP/ISocketUDP.h"
#include "Networking/API/UDP/ServerUDP.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ServerUDP::ServerUDP(const ServerUDPDesc& desc) :
		m_Desc(desc),
		m_pSocket(nullptr),
		m_PacketLoss(0.0f)
	{
		
	}

	ServerUDP::~ServerUDP()
	{
		
	}

	void ServerUDP::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateReceivingPacketLoss(lossRatio);
	}

	void ServerUDP::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateTransmittingPacketLoss(lossRatio);
	}

	ISocket* ServerUDP::SetupSocket()
	{
		ISocketUDP* pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (pSocket)
		{
			if (pSocket->Bind(GetEndPoint()))
			{
				m_Transciver.SetSocket(pSocket);
				LOG_INFO("[ServerUDP]: Started %s", GetEndPoint().ToString().c_str());
				return pSocket;
			}
		}
		return nullptr;
	}

	void ServerUDP::RunReceiver()
	{
		IPEndPoint sender;

		while (!ShouldTerminate())
		{
			if (!m_Transciver.ReceiveBegin(sender))
				continue;

			bool newConnection = false;
			ClientUDPRemote* pClient = GetOrCreateClient(sender, newConnection);

			if (newConnection)
			{
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

			pClient->OnDataReceived(&m_Transciver);
		}
	}

	void ServerUDP::TransmitPacketsForClient(ClientRemoteBase* pClient)
	{
		ClientUDPRemote* client = (ClientUDPRemote*)pClient;
		client->SendPackets(&m_Transciver);
	}

	IClientRemoteHandler* ServerUDP::CreateClientHandler()
	{
		return m_Desc.Handler->CreateClientHandler();
	}

	ClientUDPRemote* ServerUDP::GetOrCreateClient(const IPEndPoint& sender, bool& newConnection)
	{
		ClientRemoteBase* pClient = GetClient(sender);
		if (pClient)
		{
			newConnection = false;
			return (ClientUDPRemote*)pClient;
		}
		else
		{
			newConnection = true;
			return DBG_NEW ClientUDPRemote(m_Desc.PoolSize, m_Desc.MaxRetries, sender, this);
		}
	}

	void ServerUDP::OnClientDisconnected(ClientUDPRemote* pClient, bool sendDisconnectPacket)
	{
		if(sendDisconnectPacket)
			SendDisconnect(pClient);

		UnRegisterClient(pClient);
	}

	void ServerUDP::SendDisconnect(ClientUDPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_DISCONNECT));
		client->SendPackets(&m_Transciver);
	}

	void ServerUDP::SendServerFull(ClientUDPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_SERVER_FULL));
		client->SendPackets(&m_Transciver);
	}

	void ServerUDP::SendServerNotAccepting(ClientUDPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_SERVER_NOT_ACCEPTING));
		client->SendPackets(&m_Transciver);
	}

	ServerUDP* ServerUDP::Create(const ServerUDPDesc& desc)
	{
		return DBG_NEW ServerUDP(desc);
	}
}