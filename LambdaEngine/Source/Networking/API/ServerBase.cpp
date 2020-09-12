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
		m_Accepting(true),
		m_LockClientVectors()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.insert(this);
	}

	ServerBase::~ServerBase()
	{
		assert(m_Clients.empty());

		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Servers.erase(this);

		LOG_INFO("[ServerBase]: Released");
	}

	bool ServerBase::Start(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning() && ThreadsHasTerminated())
		{
			m_IPEndPoint = ipEndPoint;
			if (StartThreads())
			{
				LOG_WARNING("[ServerBase]: Starting...");
				return true;
			}
		}
		return false;
	}

	void ServerBase::Stop()
	{
		{
			std::scoped_lock<SpinLock> lock(m_LockClients);
			for (auto& pair : m_Clients)
			{
				pair.second->ReleaseByServer();
			}
			m_Clients.clear();
		}

		{
			std::scoped_lock<SpinLock> lock(m_LockClientVectors);
			m_ClientsToAdd.Clear();
			m_ClientsToRemove.Clear();
		}

		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ServerBase::Release()
	{
		TerminateAndRelease();
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
		return (uint8)m_Clients.size();
	}

	const ServerDesc& ServerBase::GetDescription() const
	{
		return m_Description;
	}

	void ServerBase::OnClientAskForTermination(ClientRemoteBase* pClient)
	{
		std::scoped_lock<SpinLock> lock(m_LockClientVectors);
		LOG_INFO("[ServerBase]: Client Added to Remove Queue");
		m_ClientsToRemove.PushBack(pClient);
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

		if (!m_ClientsToAdd.IsEmpty() || !m_ClientsToRemove.IsEmpty())
		{
			for (uint32 i = 0; i < m_ClientsToAdd.GetSize(); i++)
			{
				LOG_INFO("[ServerBase]: Client Registered");
				m_Clients.insert({ m_ClientsToAdd[i]->GetEndPoint(), m_ClientsToAdd[i] });
			}

			for (uint32 i = 0; i < m_ClientsToRemove.GetSize(); i++)
			{
				LOG_INFO("[ServerBase]: Client Unregistered");
				m_Clients.erase(m_ClientsToRemove[i]->GetEndPoint());
				m_ClientsToRemove[i]->OnTerminationApproved();
			}

			std::scoped_lock<SpinLock> lock2(m_LockClientVectors);
			m_ClientsToAdd.Clear();
			m_ClientsToRemove.Clear();
		}

		Flush();
	}

	void ServerBase::RegisterClient(ClientRemoteBase* pClient)
	{
		std::scoped_lock<SpinLock> lock(m_LockClientVectors);
		m_ClientsToAdd.PushBack(pClient);
	}

	void ServerBase::RunTransmitter()
	{
		while (!ShouldTerminate())
		{
			YieldTransmitter();
			{
				std::scoped_lock<SpinLock> lock(m_LockClients);
				for (auto& pair : m_Clients)
				{
					pair.second->TransmitPackets();
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