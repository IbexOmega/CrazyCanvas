#include "Network/API/ServerUDP.h"
#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/NetworkPacket.h"
#include "Network/API/IServerUDPHandler.h"
#include "Network/API/RemoteClientUDP.h"

#include "Log/Log.h"
#include "Threading/Thread.h"

namespace LambdaEngine
{
	ServerUDP::ServerUDP(IServerUDPHandler* handler) :
		m_pServerSocket(nullptr),
		m_pHandler(handler)
	{

	}

	ServerUDP::~ServerUDP()
	{
		LOG_WARNING("~ServerUDP()");
	}

	void ServerUDP::OnThreadStarted()
	{
		m_pServerSocket = CreateServerSocket(GetAddress(), GetPort());
		if (!m_pServerSocket)
		{
			LOG_ERROR("[ServerUDP]: Failed to start!");
			Stop();
		}
		else
		{
			LOG_INFO("[ServerUDP]: Started %s:%d", GetAddress().c_str(), GetPort());
		}
	}

	void ServerUDP::OnThreadUpdate()
	{
		NetworkPacket packet(PACKET_TYPE_UNDEFINED);
		int32 bytesReceived = 0;
		std::string address;
		uint16 port;

		while (!ShouldTerminate())
		{
			if (m_pServerSocket->ReceiveFrom(packet.GetBuffer(), MAXIMUM_PACKET_SIZE, bytesReceived, address, port))
			{
				packet.Reset();
				packet.UnPack();
				if (bytesReceived == packet.GetSize())
				{
					//LOCK
					uint64 hash = Hash(address, port);
					RemoteClientUDP* client = m_Clients[hash];
					if (!client)
					{
						IClientUDPHandler* handler = m_pHandler->CreateClientHandlerUDP();
						client = new RemoteClientUDP(address, port, hash, this, handler);
						m_Clients[hash] = client;
					}
					client->OnPacketReceived(&packet);
				}
			}
			else
			{
				Stop();
			}
		}
	}

	void ServerUDP::OnThreadTerminated()
	{
		//LOCK
		for (auto client : m_Clients)
		{
			client.second->Release();
		}
		m_Clients.clear();
		delete m_pServerSocket;
	}

	void ServerUDP::OnStopRequested()
	{
		if (m_pServerSocket)
		{
			m_pServerSocket->Close();
		}
	}

	void ServerUDP::OnReleaseRequested()
	{

	}

	ISocketUDP* ServerUDP::CreateServerSocket(const std::string& address, uint16 port)
	{
		ISocketUDP* serverSocket = PlatformSocketFactory::CreateSocketUDP();
		if (!serverSocket)
			return nullptr;

		if (!serverSocket->Bind(address, port))
		{
			serverSocket->Close();
			delete serverSocket;
			return nullptr;
		}

		return serverSocket;
	}

	uint64 ServerUDP::Hash(const std::string& address, uint64 port)
	{
		std::hash<std::string> hasher;	
		return port ^ hasher(address) + 0x9e3779b9 + (port << 6) + (port >> 2);;
	}
}