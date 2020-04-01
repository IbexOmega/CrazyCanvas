#include "Network/API/ClientTCP.h"
#include "Network/API/ISocketTCP.h"
#include "Network/API/ServerTCP.h"
#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/IClientTCPHandler.h"

#include "Threading/Thread.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientTCP::ClientTCP(IClientTCPHandler* handler) : ClientTCP(handler, nullptr)
	{

	}

	ClientTCP::ClientTCP(IClientTCPHandler* handler, ISocketTCP* socket) :
		m_pHandler(handler),
		m_pSocket(socket),
		m_Stop(false)
	{
		m_ServerSide = socket != nullptr;
	}

	bool ClientTCP::Connect(const std::string& address, uint16 port)
	{
		if (m_pSocket)
		{
			LOG_ERROR_CRIT("Tried to connect an already connected Client!");
			return false;
		}

		m_pThread = Thread::Create(std::bind(&ClientTCP::Run, this, address, port), std::bind(&ClientTCP::OnStoped, this));
		return true;
	}

	void ClientTCP::Disconnect()
	{
		m_Stop = true;
	}

	bool ClientTCP::IsServerSide() const
	{
		return m_ServerSide;
	}

	void ClientTCP::Run(std::string address, uint16 port)
	{
		m_pSocket = CreateClientSocket(address, port);
		if (!m_pSocket)
		{
			LOG_ERROR_CRIT("Failed to connect to Server!");
			m_pHandler->OnClientFailedConnecting(this);
			return;
		}

		m_pHandler->OnClientConnected(this);

		while (!m_Stop)
		{
			
		}

		m_pSocket->Close();

		m_pSocket = nullptr;
		m_pHandler->OnClientDisconnected(this);
	}

	void ClientTCP::OnStoped()
	{
		m_pThread = nullptr;
		delete m_pSocket;
		m_pSocket = nullptr;
	}

	ISocketTCP* ClientTCP::CreateClientSocket(const std::string& address, uint16 port)
	{
		ISocketTCP* socket = PlatformSocketFactory::CreateSocketTCP();
		if (!socket)
			return nullptr;

		if (!socket->Connect(address, port))
		{
			socket->Close();
			delete socket;
			return nullptr;
		}

		return socket;
	}
}