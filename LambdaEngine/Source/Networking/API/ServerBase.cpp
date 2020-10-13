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

	void ServerBase::Stop(const std::string& reason)
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
			for (ClientRemoteBase* pClient : m_ClientsToAdd)
			{
				pClient->ReleaseByServer();
			}
			m_ClientsToAdd.Clear();
			m_ClientsToRemove.Clear();
		}

		TerminateThreads(reason);

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ServerBase::Release()
	{
		TerminateAndRelease("Release Requested");
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

	const ClientMap& ServerBase::GetClients() const
	{
		return m_Clients;
	}

	void ServerBase::OnClientAskForTermination(ClientRemoteBase* pClient)
	{
		std::scoped_lock<SpinLock> lock(m_LockClientVectors);
		LOG_INFO("[ServerBase]: Client Added to Remove Queue");
		m_ClientsToRemove.PushBack(pClient);
	}

	bool ServerBase::SendReliableBroadcast(ClientRemoteBase* pClient, NetworkSegment* pPacket, IPacketListener* pListener)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		bool result = true;

		if (!pClient->SendReliable(pPacket, pListener))
			result = false;

		for (auto& pair : m_Clients)
		{ 
			if (pair.second != pClient)
			{
				NetworkSegment* pPacketDuplicate = pair.second->GetFreePacket(pPacket->GetType());
				pPacket->CopyTo(pPacketDuplicate);
				if (!pair.second->SendReliable(pPacketDuplicate, pListener))
					result = false;
			}
		}

		return result;
	}

	bool ServerBase::SendUnreliableBroadcast(ClientRemoteBase* pClient, NetworkSegment* pPacket)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		bool result = true;

		if (!pClient->SendUnreliable(pPacket))
			result = false;

		for (auto& pair : m_Clients)
		{
			if (pair.second != pClient)
			{
				NetworkSegment* pPacketDuplicate = pair.second->GetFreePacket(pPacket->GetType());
				pPacket->CopyTo(pPacketDuplicate);
				if (!pair.second->SendUnreliable(pPacketDuplicate))
					result = false;
			}
		}

		return result;
	}

	ClientRemoteBase* ServerBase::GetClient(const IPEndPoint& endPoint)
	{
		std::scoped_lock<SpinLock> lock(m_LockClients);
		auto pIterator = m_Clients.find(endPoint);
		if (pIterator != m_Clients.end())
		{
			return pIterator->second;
		}

		if (!m_ClientsToAdd.IsEmpty())
		{
			std::scoped_lock<SpinLock> lock2(m_LockClientVectors);
			for (uint32 i = 0; i < m_ClientsToAdd.GetSize(); i++)
			{
				if (m_ClientsToAdd[i]->GetEndPoint() == endPoint)
					return m_ClientsToAdd[i];
			}
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
			std::scoped_lock<SpinLock> lock(m_LockClientVectors);
			m_ClientsToAdd.PushBack(pClient);
		}
	}

	bool ServerBase::OnThreadsStarted(std::string& reason)
	{
		m_pSocket = SetupSocket(reason);
		return m_pSocket;
	}

	void ServerBase::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
		{
			m_pSocket->Close();
			delete m_pSocket;
			m_pSocket = nullptr;
		}
		LOG_INFO("[ServerBase]: Stopped");
	}

	void ServerBase::OnTerminationRequested(const std::string& reason)
	{
		LOG_WARNING("[ServerBase]: Stopping... [%s]", reason.c_str());
	}

	void ServerBase::OnReleaseRequested(const std::string& reason)
	{
		Stop(reason);
	}

	void ServerBase::FixedTick(Timestamp delta)
	{
		{
			std::scoped_lock<SpinLock> lock(m_LockClients);
			for (auto& pair : m_Clients)
			{
				pair.second->FixedTick(delta);
			}

			if (!m_ClientsToAdd.IsEmpty() || !m_ClientsToRemove.IsEmpty())
			{
				TArray<ClientRemoteBase*> clientsApproved;
				TArray<ClientRemoteBase*> unconnectedClientsToTick;
				{
					std::scoped_lock<SpinLock> lock2(m_LockClientVectors);

					for (uint32 i = 0; i < m_ClientsToRemove.GetSize(); i++)
					{
						LOG_INFO("[ServerBase]: Client Unregistered");

						for (int32 j = m_ClientsToAdd.GetSize() - 1; j >= 0; j--)
							if (m_ClientsToAdd[j] == m_ClientsToRemove[i])
								m_ClientsToAdd.Erase(m_ClientsToAdd.Begin() + j);

						m_Clients.erase(m_ClientsToRemove[i]->GetEndPoint());
						m_ClientsToRemove[i]->OnTerminationApproved();
					}

					for (int32 i = m_ClientsToAdd.GetSize() - 1; i >= 0; i--)
					{
						ClientRemoteBase* pClient = m_ClientsToAdd[i];
						unconnectedClientsToTick.PushBack(pClient);
						
						if (pClient->IsConnected())
						{
							LOG_INFO("[ServerBase]: Client Registered");
							m_Clients.insert({ pClient->GetEndPoint(), pClient });
							m_ClientsToAdd.Erase(m_ClientsToAdd.Begin() + i);
							clientsApproved.PushBack(pClient);
						}
					}	

					m_ClientsToRemove.Clear();
				}
				
				for (ClientRemoteBase* pClient : unconnectedClientsToTick)
				{
					pClient->FixedTick(delta);
				}

				for (ClientRemoteBase* pClient : clientsApproved)
				{
					pClient->OnConnectionApproved();
				}
			}
		}
		
		Flush();
	}

	void ServerBase::RunTransmitter()
	{
		while (!ShouldTerminate())
		{
			YieldTransmitter();
			{
				std::scoped_lock<SpinLock> lock(m_LockClients);
				for (auto& pair : m_Clients)
					pair.second->TransmitPackets();

				if (!m_ClientsToAdd.IsEmpty())
				{
					std::scoped_lock<SpinLock> lock2(m_LockClientVectors);
					for (ClientRemoteBase* pClient : m_ClientsToAdd)
						pClient->TransmitPackets();
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
				server->FixedTick(delta);
			}
		}
	}
}