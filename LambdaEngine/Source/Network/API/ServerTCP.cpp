#include "Network/API/ServerTCP.h"
#include "Network/API/PlatformNetworkUtils.h"
#include "Network/API/IServerTCPHandler.h"
#include "Network/API/ClientTCP.h"
#include "Network/API/NetworkPacket.h"

#include "Threading/Thread.h"

#include "Log/Log.h"

#include <algorithm>

namespace LambdaEngine
{
	ServerTCP::ServerTCP(uint16 maxClients, IServerTCPHandler* handler) :
		m_pServerSocket(nullptr),
		m_MaxClients(maxClients),
		m_pHandler(handler)
	{

	}

	ServerTCP::~ServerTCP()
	{
		LOG_WARNING("~ServerTCP()");
	}

	uint8 ServerTCP::GetNrOfClients() const
	{
		return uint8(m_Clients.size());
	}

	/*******************************************
	*				PROTECTED				   *
	********************************************/

	void ServerTCP::OnThreadStarted()
	{
		m_pServerSocket = CreateServerSocket(GetAddress(), GetPort());
		if (!m_pServerSocket)
		{
			LOG_ERROR("[ServerTCP]: Failed to start!");
			Stop();
		}
		else
		{
			LOG_INFO("[ServerTCP]: Started %s:%d", GetAddress().c_str(), GetPort());
		}
	}

	void ServerTCP::OnThreadUpdate()
	{
		ISocketTCP* socket = m_pServerSocket->Accept();
		if (socket)
		{
			IClientTCPHandler* handler = m_pHandler->CreateClientHandlerTCP();
			HandleNewClient(DBG_NEW ClientTCP(handler, this, socket));
		}
		else
		{
			Stop();
		}
	}

	/*
	* Called by the Server Thread it has terminated
	*/
	void ServerTCP::OnThreadTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		for (ClientTCP* client : m_Clients)
		{
			client->Release();
		}
		m_Clients.clear();
		delete m_pServerSocket;
	}

	void ServerTCP::OnStopRequested()
	{
		if (m_pServerSocket)
		{
			m_pServerSocket->Close();
		}
	}

	void ServerTCP::OnReleaseRequested()
	{

	}

	void ServerTCP::OnClientDisconnected(ClientTCP* client)
	{
		RemoveClient(client);
		m_pHandler->OnClientDisconnectedTCP(client);
		client->Release();
	}

	/*******************************************
	*					PRIVATE				   *
	********************************************/

	void ServerTCP::HandleNewClient(ClientTCP* client)
	{
		if (m_Clients.size() < m_MaxClients)
		{
			if (m_pHandler->OnClientAcceptedTCP(client))
			{
				AddClient(client);
				m_pHandler->OnClientConnectedTCP(client);
				return;
			}
		}
		else
		{
			NetworkPacket packet(PACKET_TYPE_SERVER_FULL, false);
			client->SendPacketImmediately(&packet);
		}

		client->Release();
	}

	void ServerTCP::AddClient(ClientTCP* client)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_Clients.push_back(client);
	}

	void ServerTCP::RemoveClient(ClientTCP* client)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_Clients.erase(std::remove(m_Clients.begin(), m_Clients.end(), client), m_Clients.end());
	}

	ISocketTCP* ServerTCP::CreateServerSocket(const std::string& address, uint16 port)
	{
		ISocketTCP* serverSocket = PlatformNetworkUtils::CreateSocketTCP();
		if (!serverSocket)
			return nullptr;

		if (!serverSocket->Bind(address, port))
		{
			serverSocket->Close();
			delete serverSocket;
			return nullptr;
		}

		if (!serverSocket->Listen())
		{
			serverSocket->Close();
			delete serverSocket;
			return nullptr;
		}
			
		return serverSocket;
	}
}