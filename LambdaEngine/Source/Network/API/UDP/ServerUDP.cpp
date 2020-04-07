#include "Network/API/UDP/ServerUDP.h"
#include "Network/API/UDP/IServerUDPHandler.h"
#include "Network/API/UDP/ClientUDPRemote.h"

#include "Network/API/PlatformNetworkUtils.h"
#include "Network/API/NetworkPacket.h"

#include "Log/Log.h"
#include "Threading/Thread.h"

namespace LambdaEngine
{
	ServerUDP::ServerUDP(IServerUDPHandler* handler) :
		ClientBase(true),
		m_pServerSocket(nullptr),
		m_pHandler(handler)
	{
	}

	ServerUDP::~ServerUDP()
	{
		LOG_WARNING("~ServerUDP()");
	}

	void ServerUDP::OnReleaseRequested()
	{
		Stop();
	}

	bool ServerUDP::TransmitPacket(NetworkPacket* packet)
	{
		char* buffer = packet->GetBuffer();
		int32 bytesToSend = packet->GetSize();
		int32 bytesSent = 0;
		if (!m_pServerSocket->SendTo(buffer, bytesToSend, bytesSent, packet->GetAddress(), packet->GetPort()))
		{
			return false;
		}
		return bytesToSend == bytesSent;
	}

	void ServerUDP::OnClientReleased(ClientUDPRemote* client)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_Clients.erase(client->GetHash());
	}

	void ServerUDP::Stop()
	{
		if (m_pServerSocket)
		{
			m_pServerSocket->Close();
		}
		TerminateThreads();
	}

	void ServerUDP::Release()
	{
		ClientBase::Release();
	}

	bool ServerUDP::Start(const std::string& address, uint16 port)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (ThreadsHaveTerminated())
		{
			SetAddressAndPort(address, port);
			return StartThreads();
		}
		return false;
	}

	void ServerUDP::OnTransmitterStarted()
	{
		m_pServerSocket = CreateServerSocket(GetAddress(), GetPort());
		if (!m_pServerSocket)
		{
			LOG_ERROR("[ServerUDP]: Failed to start!");
			Stop();
		}
		else
		{
			if (GetAddress() == ADDRESS_BROADCAST)
			{
				m_pServerSocket->EnableBroadcast();
			}
			LOG_INFO("[ServerUDP]: Started %s:%d", GetAddress().c_str(), GetPort());
		}
	}

	void ServerUDP::OnReceiverStarted()
	{

	}

	void ServerUDP::UpdateReceiver(NetworkPacket* packet)
	{
		int32 bytesReceived = 0;
		std::string address;
		uint16 port;

		while (!ShouldTerminate())
		{
			if (m_pServerSocket->ReceiveFrom(m_ReceiveBuffer, MAXIMUM_DATAGRAM_SIZE, bytesReceived, address, port))
			{
				memcpy(packet->GetBuffer(), m_ReceiveBuffer, sizeof(PACKET_SIZE));
				packet->Reset();
				packet->UnPack();
				if (bytesReceived == packet->GetSize())
				{
					memcpy(packet->GetBuffer(), m_ReceiveBuffer, bytesReceived);
					uint64 hash = Hash(address, port);
					ClientUDPRemote* client = nullptr;
					{
						std::scoped_lock<SpinLock> lock(m_Lock);
						client = m_Clients[hash];
					}
					if (!client)
					{
						IClientUDPHandler* handler = m_pHandler->CreateClientHandlerUDP();
						client = DBG_NEW ClientUDPRemote(address, port, hash, this, handler);
						std::scoped_lock<SpinLock> lock(m_Lock);
						m_Clients[hash] = client;
					}
					client->OnPacketReceived(packet);
				}
				else
				{
					LOG_ERROR("Error received a packet with the wrong size %i | %i", bytesReceived, packet->GetSize());
				}
				RegisterPacketsReceived(1);
			}
			else
			{
				Stop();
			}
		}
	}

	void ServerUDP::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		for (auto client : m_Clients)
		{
			client.second->ReleaseInternal();
		}
		m_Clients.clear();
		delete m_pServerSocket;
		m_pServerSocket = nullptr;
	}

	ISocketUDP* ServerUDP::CreateServerSocket(const std::string& address, uint16 port)
	{
		ISocketUDP* serverSocket = PlatformNetworkUtils::CreateSocketUDP();
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