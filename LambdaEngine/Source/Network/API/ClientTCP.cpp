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

		if(m_ServerSide)
			m_pThread = Thread::Create(std::bind(&ClientTCP::Run, this, "", 0), std::bind(&ClientTCP::OnStoped, this));
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

	void ClientTCP::SendPacket(const std::string& data)
	{
		////TEMP
		uint32 bytesSent;
		m_pSocket->Send(data.c_str(), data.length(), bytesSent);
	}

	bool ClientTCP::IsServerSide() const
	{
		return m_ServerSide;
	}

	void ClientTCP::Run(std::string address, uint16 port)
	{
		if (!IsServerSide())
		{
			m_pSocket = CreateClientSocket(address, port);
			if (!m_pSocket)
			{
				LOG_ERROR_CRIT("Failed to connect to Server!");
				m_pHandler->OnClientFailedConnecting(this);
				return;
			}
		}

		m_pHandler->OnClientConnected(this);

		char buffer[256];
		uint32 bytesReceived;
		while (!m_Stop)
		{
			if (m_pSocket->Receive(buffer, 256, bytesReceived))
			{
				std::string message(buffer, bytesReceived);
				LOG_MESSAGE(message.c_str());
			}
			else
			{
				Disconnect();
			}
		}
		m_pSocket->Close();
	}

	void ClientTCP::OnStoped()
	{
		m_pThread = nullptr;
		delete m_pSocket;
		m_pSocket = nullptr;
		m_pHandler->OnClientDisconnected(this);
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