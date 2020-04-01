#include "Network/API/ServerTCP.h"
#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/IServerTCPHandler.h"
#include "Network/API/ClientTCP.h"

#include "Threading/Thread.h"

#include "Log/Log.h"

#include <algorithm>

namespace LambdaEngine
{
	ServerTCP::ServerTCP(IServerTCPHandler* handler) :
		m_pServerSocket(nullptr),
		m_pThread(nullptr),
		m_Stop(false),
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
			LOG_ERROR_CRIT("Tried to start an already running Server!");
			return false;
		}

		m_Stop = false;

		m_pThread = Thread::Create(std::bind(&ServerTCP::Run, this, address, port), std::bind(&ServerTCP::OnStoped, this));
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

	void ServerTCP::Run(std::string address, uint16 port)
	{
		m_pServerSocket = CreateServerSocket(address, port);
		if (!m_pServerSocket)
		{
			LOG_ERROR_CRIT("Failed to start Server!");
			return;
		}

		while (!m_Stop)
		{
			ISocketTCP* socket = m_pServerSocket->Accept();
			if(socket)
			{
				ClientTCP* client = new ClientTCP(this, socket);
				if (m_pHandler->OnClientAccepted(client))
				{
					m_Clients.push_back(client);
					m_pHandler->OnClientConnected(client);
				}
				else
				{
					socket->Close();
					delete socket;
					delete client;
				}				
			}
		}

		for (ClientTCP* client : m_Clients)
		{
			client->Disconnect();
		}

		m_Clients.clear();
		m_pServerSocket->Close();
	}

	void ServerTCP::OnStoped()
	{
		m_pThread = nullptr;
		delete m_pServerSocket;
		m_pServerSocket = nullptr;
	}

	void ServerTCP::OnClientConnected(ClientTCP* client)
	{

	}

	void ServerTCP::OnClientDisconnected(ClientTCP* client)
	{
		m_Clients.erase(std::remove(m_Clients.begin(), m_Clients.end(), client), m_Clients.end()); //Lock?
		m_pHandler->OnClientDisconnected(client);
		delete client;
	}

	void ServerTCP::OnClientFailedConnecting(ClientTCP* client)
	{

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