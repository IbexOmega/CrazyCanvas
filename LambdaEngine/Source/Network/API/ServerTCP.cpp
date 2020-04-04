#include "Network/API/ServerTCP.h"
#include "Network/API/PlatformSocketFactory.h"
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
		m_pThread(nullptr),
		m_Stop(false),
		m_MaxClients(maxClients),
		m_pHandler(handler)
	{

	}

	ServerTCP::~ServerTCP()
	{

	}

	bool ServerTCP::Start(const std::string& address, uint16 port)
	{
		if (IsRunning())
		{
			LOG_ERROR("[ServerTCP]: Tried to start an already running Server!");
			return false;
		}

		m_Stop = false;

		m_pThread = Thread::Create(std::bind(&ServerTCP::Run, this, address, port), std::bind(&ServerTCP::OnStopped, this));
		return true;
	}

	void ServerTCP::Stop()
	{
		if (IsRunning())
		{
			m_Stop = true;
		}
	}

	bool ServerTCP::IsRunning() const
	{
		return m_pThread != nullptr;
	}

	const std::string& ServerTCP::GetAddress()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pServerSocket)
			return m_pServerSocket->GetAddress();
		return "";
	}

	uint16 ServerTCP::GetPort()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pServerSocket)
			return m_pServerSocket->GetPort();
		return 0;
	}

	void ServerTCP::Run(std::string address, uint16 port)
	{
		m_pServerSocket = CreateServerSocket(address, port);
		if (!m_pServerSocket)
		{
			LOG_ERROR("[ServerTCP]: Failed to start!");
			return;
		}

		LOG_INFO("[ServerTCP]: Started %s:%d", address.c_str(), port);

		while (!m_Stop)
		{
			ISocketTCP* socket = m_pServerSocket->Accept();
			if(socket)
			{
				HandleNewClient(new ClientTCP(this, socket));
			}
			else
			{
				Stop(); //..............
			}
		}

		for (ClientTCP* client : m_Clients)
		{
			client->Disconnect();
		}

		ClearClients();

		m_pServerSocket->Close();
	}

	void ServerTCP::OnStopped()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pThread = nullptr;
		delete m_pServerSocket;
		m_pServerSocket = nullptr;
		LOG_WARNING("[ServerTCP]: Stopped");
	}

	void ServerTCP::HandleNewClient(ClientTCP* client)
	{
		if (m_Clients.size() < m_MaxClients)
		{
			if (m_pHandler->OnClientAccepted(client))
			{
				AddClient(client);
				m_pHandler->OnClientConnected(client);
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

	void ServerTCP::ClearClients()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_Clients.clear();
	}

	uint8 ServerTCP::GetNrOfClients() const
	{
		return m_Clients.size();
	}

	void ServerTCP::OnClientConnected(ClientTCP* client)
	{
		//LOG_INFO("[ServerTCP]: Client Connected");
	}

	void ServerTCP::OnClientDisconnected(ClientTCP* client)
	{
		RemoveClient(client);
		m_pHandler->OnClientDisconnected(client);
		client->Release();
		//LOG_WARNING("[ServerTCP]: Client Disconnected");
	}

	void ServerTCP::OnClientFailedConnecting(ClientTCP* client)
	{

	}

	void ServerTCP::OnClientPacketReceived(ClientTCP* client, NetworkPacket* packet)
	{
		if (packet->ReadPacketType() == PACKET_TYPE_USER_DATA)
		{
			std::string str;
			packet->ReadString(str);
			LOG_MESSAGE(str.c_str());
		}
		else if (packet->ReadPacketType() == PACKET_TYPE_PING)
		{
			LOG_MESSAGE("Pong()");
		}
	}

	ISocketTCP* ServerTCP::CreateServerSocket(const std::string& address, uint16 port)
	{
		ISocketTCP* serverSocket = PlatformSocketFactory::CreateSocketTCP();
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
