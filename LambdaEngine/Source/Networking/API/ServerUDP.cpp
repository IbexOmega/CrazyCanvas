#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerUDP.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/ClientUDPRemote.h"
#include "Networking/API/IServerUDPHandler.h"

#include "Log/Log.h"

#include "Networking/API/BinaryEncoder.h"

namespace LambdaEngine
{
	std::set<ServerUDP*> ServerUDP::s_Servers;
	SpinLock ServerUDP::s_Lock;

	ServerUDP::ServerUDP(IServerUDPHandler* pHandler, uint8 maxClients, uint16 packetPerClient) :
		m_pHandler(pHandler),
		m_MaxClients(maxClients),
		m_PacketsPerClient(packetPerClient),
		m_pSocket(nullptr),
		m_Accepting(true)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.insert(this);
	}

	ServerUDP::~ServerUDP()
	{
		for (auto& pair : m_Clients)
		{
			delete pair.second;
		}
		m_Clients.clear();

		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.erase(this);

		LOG_INFO("[ServerUDP]: Released");
	}

	bool ServerUDP::Start(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				m_IPEndPoint = ipEndPoint;
				LOG_WARNING("[ServerUDP]: Starting...");
				return true;
			}
		}
		return false;
	}

	void ServerUDP::Stop()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ServerUDP::Release()
	{
		NetWorker::TerminateAndRelease();
	}

	bool ServerUDP::IsRunning()
	{
		return ThreadsAreRunning() && !ShouldTerminate();
	}

	const IPEndPoint& ServerUDP::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void ServerUDP::SetAcceptingConnections(bool accepting)
	{
		m_Accepting = accepting;
	}

	bool ServerUDP::IsAcceptingConnections()
	{
		return m_Accepting;
	}

	bool ServerUDP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(m_IPEndPoint))
			{
				LOG_INFO("[ServerUDP]: Started %s", m_IPEndPoint.ToString().c_str());
				return true;
			}
		}
		return false;
	}

	void ServerUDP::RunReceiver()
	{
		int32 bytesReceived = 0;
		IPEndPoint sender;

		while (!ShouldTerminate())
		{
			if (!m_pSocket->ReceiveFrom(m_pReceiveBuffer, UINT16_MAX, bytesReceived, sender))
			{
				TerminateThreads();
				break;
			}

			ClientUDPRemote* pClient = nullptr;
			{
				std::scoped_lock<SpinLock> lock(m_LockClients);
				auto pIterator = m_Clients.find(sender);
				if (pIterator != m_Clients.end())
				{
					pClient = pIterator->second;
				}
				else
				{
					pClient = DBG_NEW ClientUDPRemote(m_PacketsPerClient, sender, this);
					if (!IsAcceptingConnections())
					{
						SendServerNotAccepting(pClient);
						pClient->Release();
						continue;
					}
					else if(m_Clients.size() >= m_MaxClients)
					{
						SendServerFull(pClient);
						pClient->Release();
						continue;
					}
					else
					{
						m_Clients.insert({ sender, pClient });
					}
				}
			}
			pClient->OnDataReceived(m_pReceiveBuffer, bytesReceived);
		}
	}

	void ServerUDP::RunTranmitter()
	{
		while (!ShouldTerminate())
		{
			YieldTransmitter();
			{
				std::scoped_lock<SpinLock> lock(m_LockClients);
				for (auto& tuple : m_Clients)
				{
					tuple.second->SendPackets();
				}
			}
		}
	}
	
	void ServerUDP::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ServerUDP]: Stopped");
	}

	void ServerUDP::OnTerminationRequested()
	{
		LOG_WARNING("[ServerUDP]: Stopping...");
	}

	void ServerUDP::OnReleaseRequested()
	{
		Stop();
	}

	void ServerUDP::Transmit(const IPEndPoint& ipEndPoint, const char* data, int32 bytesToWrite)
	{
		int32 bytesSent = 0;
		if (!m_pSocket->SendTo(data, bytesToWrite, bytesSent, ipEndPoint))
		{
			TerminateThreads();
		}
		else if (bytesToWrite != bytesSent)
		{
			TerminateThreads();
		}
	}

	IClientUDPHandler* ServerUDP::CreateClientUDPHandler()
	{
		return m_pHandler->CreateClientUDPHandler();
	}

	void ServerUDP::OnClientDisconnected(ClientUDPRemote* client, bool sendDisconnectPacket)
	{
		if(sendDisconnectPacket)
			SendDisconnect(client);

		std::scoped_lock<SpinLock> lock(m_LockClients);
		m_Clients.erase(client->GetEndPoint());
	}

	void ServerUDP::SendDisconnect(ClientUDPRemote* client)
	{
		client->SendUnreliable(client->GetFreePacket(NetworkPacket::TYPE_DISCONNECT));
		client->SendPackets();
	}

	void ServerUDP::SendServerFull(ClientUDPRemote* client)
	{
		client->SendUnreliable(client->GetFreePacket(NetworkPacket::TYPE_SERVER_FULL));
		client->SendPackets();
	}

	void ServerUDP::SendServerNotAccepting(ClientUDPRemote* client)
	{
		client->SendUnreliable(client->GetFreePacket(NetworkPacket::TYPE_SERVER_NOT_ACCEPTING));
		client->SendPackets();
	}

	ServerUDP* ServerUDP::Create(IServerUDPHandler* pHandler, uint8 maxClients, uint16 packetsPerClient)
	{
		return DBG_NEW ServerUDP(pHandler, maxClients, packetsPerClient);
	}

	void ServerUDP::FixedTickStatic(Timestamp timestamp)
	{
		if (!s_Servers.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (ServerUDP* server : s_Servers)
			{
				server->Flush();
			}
		}
	}
}