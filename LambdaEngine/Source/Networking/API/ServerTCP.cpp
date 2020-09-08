#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerTCP.h"
#include "Networking/API/ISocketTCP.h"
#include "Networking/API/ClientTCPRemote.h"
#include "Networking/API/IServerHandler.h"
#include "Networking/API/BinaryEncoder.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	std::set<ServerTCP*> ServerTCP::s_Servers;
	SpinLock ServerTCP::s_Lock;

	ServerTCP::ServerTCP(const ServerTCPDesc& desc) :
		m_Desc(desc),
		m_pSocket(nullptr),
		m_Accepting(true),
		m_PacketLoss(0.0f)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.insert(this);
	}

	ServerTCP::~ServerTCP()
	{
		for (auto& pair : m_Clients)
		{
			delete pair.second;
		}
		m_Clients.clear();

		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.erase(this);

		LOG_INFO("[ServerTCP]: Released");
	}

	bool ServerTCP::Start(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				m_IPEndPoint = ipEndPoint;
				LOG_WARNING("[ServerTCP]: Starting...");
				return true;
			}
		}
		return false;
	}

	void ServerTCP::Stop()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ServerTCP::Release()
	{
		NetWorker::TerminateAndRelease();
	}

	bool ServerTCP::IsRunning()
	{
		return ThreadsAreRunning() && !ShouldTerminate();
	}

	const IPEndPoint& ServerTCP::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void ServerTCP::SetAcceptingConnections(bool accepting)
	{
		m_Accepting = accepting;
	}

	bool ServerTCP::IsAcceptingConnections()
	{
		return m_Accepting;
	}

	void ServerTCP::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateReceivingPacketLoss(lossRatio);
	}

	void ServerTCP::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateTransmittingPacketLoss(lossRatio);
	}

	bool ServerTCP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketTCP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(m_IPEndPoint))
			{
				m_Transciver.SetSocket(m_pSocket);
				LOG_INFO("[ServerTCP]: Started %s", m_IPEndPoint.ToString().c_str());
				return true;
			}
		}
		return false;
	}

	void ServerTCP::RunReceiver()
	{
		IPEndPoint sender;

		while (!ShouldTerminate())
		{
			if (!m_Transciver.ReceiveBegin(sender))
				continue;

			bool newConnection = false;
			ClientTCPRemote* pClient = GetOrCreateClient(sender, newConnection);

			if (newConnection)
			{
				if (!IsAcceptingConnections())
				{
					SendServerNotAccepting(pClient);
					pClient->Release();
					continue;
				}
				else if (m_Clients.size() >= m_Desc.MaxClients)
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

			pClient->OnDataReceived(&m_Transciver);
		}
	}

	void ServerTCP::RunTransmitter()
	{
		while (!ShouldTerminate())
		{
			YieldTransmitter();
			{
				std::scoped_lock<SpinLock> lock(m_LockClients);
				for (auto& tuple : m_Clients)
				{
					tuple.second->SendPackets(&m_Transciver);
				}
			}
		}
	}
	
	void ServerTCP::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ServerTCP]: Stopped");
	}

	void ServerTCP::OnTerminationRequested()
	{
		LOG_WARNING("[ServerTCP]: Stopping...");
	}

	void ServerTCP::OnReleaseRequested()
	{
		Stop();
	}

	IClientRemoteHandler* ServerTCP::CreateClientHandler()
	{
		return m_Desc.Handler->CreateClientHandler();
	}

	ClientTCPRemote* ServerTCP::GetOrCreateClient(const IPEndPoint& sender, bool& newConnection)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		auto pIterator = m_Clients.find(sender);
		if (pIterator != m_Clients.end())
		{
			newConnection = false;
			return pIterator->second;
		}
		else
		{
			newConnection = true;
			return DBG_NEW ClientTCPRemote(m_Desc.PoolSize, m_Desc.MaxRetries, sender, this);
		}
	}

	void ServerTCP::OnClientDisconnected(ClientTCPRemote* client, bool sendDisconnectPacket)
	{
		if(sendDisconnectPacket)
			SendDisconnect(client);

		std::scoped_lock<SpinLock> lock(m_LockClients);
		m_Clients.erase(client->GetEndPoint());
	}

	void ServerTCP::SendDisconnect(ClientTCPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_DISCONNECT));
		client->SendPackets(&m_Transciver);
	}

	void ServerTCP::SendServerFull(ClientTCPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_SERVER_FULL));
		client->SendPackets(&m_Transciver);
	}

	void ServerTCP::SendServerNotAccepting(ClientTCPRemote* client)
	{
		client->m_PacketManager.EnqueueSegmentUnreliable(client->GetFreePacket(NetworkSegment::TYPE_SERVER_NOT_ACCEPTING));
		client->SendPackets(&m_Transciver);
	}

	void ServerTCP::Tick(Timestamp delta)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		for (auto& pair : m_Clients)
		{
			pair.second->Tick(delta);
		}
		Flush();
	}

	ServerTCP* ServerTCP::Create(const ServerTCPDesc& desc)
	{
		return DBG_NEW ServerTCP(desc);
	}

	void ServerTCP::FixedTickStatic(Timestamp timestamp)
	{
		UNREFERENCED_VARIABLE(timestamp);

		if (!s_Servers.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (ServerTCP* server : s_Servers)
			{
				server->Tick(timestamp);
			}
		}
	}
}