#include "Networking/API/ServerBase.h"
#include "Networking/API/ISocket.h"
#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/IServerHandler.h"

namespace LambdaEngine
{
	std::set<ServerBase*> ServerBase::s_Servers;
	SpinLock ServerBase::s_Lock;

	ServerBase::ServerBase(const ServerDesc& desc) :
		m_Description(desc),
		m_pSocket(nullptr),
		m_Accepting(true)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.insert(this);
	}

	ServerBase::~ServerBase()
	{
		for (auto& pair : m_Clients)
		{
			delete pair.second;
		}
		m_Clients.clear();

		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.erase(this);

		LOG_INFO("[ServerBase]: Released");
	}

	bool ServerBase::Start(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				m_IPEndPoint = ipEndPoint;
				LOG_WARNING("[ServerBase]: Starting...");
				return true;
			}
		}
		return false;
	}

	void ServerBase::Stop()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ServerBase::Release()
	{
		NetWorker::TerminateAndRelease();
	}

	bool ServerBase::IsRunning()
	{
		return ThreadsAreRunning() && !ShouldTerminate();
	}

	const IPEndPoint& ServerBase::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void ServerBase::SetAcceptingConnections(bool accepting)
	{
		m_Accepting = accepting;
	}

	bool ServerBase::IsAcceptingConnections()
	{
		return m_Accepting;
	}

	uint8 ServerBase::GetClientCount()
	{
		return m_Clients.size();
	}

	const ServerDesc& ServerBase::GetDescription() const
	{
		return m_Description;
	}

	void ServerBase::OnClientDisconnected(ClientRemoteBase* pClient)
	{
		UnRegisterClient(pClient);
	}

	ClientRemoteBase* ServerBase::GetClient(const IPEndPoint& endPoint)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		auto pIterator = m_Clients.find(endPoint);
		if (pIterator != m_Clients.end())
		{
			return pIterator->second;
		}
		return nullptr;
	}

	void ServerBase::HandleNewConnection(ClientRemoteBase* pClient)
	{
		if (!IsAcceptingConnections())
		{
			pClient->SendServerNotAccepting();
			pClient->Release();
		}
		else if (GetClientCount() >= GetDescription().MaxClients)
		{
			pClient->SendServerFull();
			pClient->Release();
		}
		else
		{
			RegisterClient(pClient);
		}
	}

	bool ServerBase::OnThreadsStarted()
	{
		m_pSocket = SetupSocket();
		return m_pSocket;
	}

	void ServerBase::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ServerBase]: Stopped");
	}

	void ServerBase::OnTerminationRequested()
	{
		LOG_WARNING("[ServerBase]: Stopping...");
	}

	void ServerBase::OnReleaseRequested()
	{
		Stop();
	}

	void ServerBase::Tick(Timestamp delta)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		for (auto& pair : m_Clients)
		{
			pair.second->Tick(delta);
		}
		Flush();
	}

	void ServerBase::RegisterClient(ClientRemoteBase* pClient)
	{
		LOG_INFO("[ServerBase]: Client Registered");
		std::scoped_lock<SpinLock> lock(m_LockClients);
		m_Clients.insert({ pClient->GetEndPoint(), pClient });
	}

	void ServerBase::UnRegisterClient(ClientRemoteBase* pClient)
	{
		LOG_INFO("[ServerBase]: Client UnRegistered");
		std::scoped_lock<SpinLock> lock(m_LockClients);
		m_Clients.erase(pClient->GetEndPoint());
	}

	void ServerBase::RunTransmitter()
	{
		while (!ShouldTerminate())
		{
			YieldTransmitter();
			{
				std::scoped_lock<SpinLock> lock(m_LockClients);
				for (auto& tuple : m_Clients)
				{
					tuple.second->TransmitPackets();
				}
			}
		}
	}

	IClientRemoteHandler* ServerBase::CreateClientHandler() const
	{
		return GetDescription().Handler->CreateClientHandler();
	}

	void ServerBase::FixedTickStatic(Timestamp delta)
	{
		if (!s_Servers.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (ServerBase* server : s_Servers)
			{
				server->Tick(delta);
			}
		}
	}
}