#include "ServerUDP.h"
#include "Log/Log.h"
#include "Threading/Thread.h"
#include "PlatformSocketFactory.h"
#include "NetworkPacket.h"

namespace LambdaEngine
{
	ServerUDP::ServerUDP(IServerUDPHandler* handler)
	{
	}

	ServerUDP::~ServerUDP()
	{
	}

	bool ServerUDP::Start(const std::string& address, uint16 port)
	{
		if (IsRunning())
		{
			LOG_ERROR("[ServerUDP]: Tried to start an already running Server!");
			return false;
		}

		m_Stop = false;

		m_pThread = Thread::Create(std::bind(&ServerUDP::Run, this, address, port), std::bind(&ServerUDP::OnStopped, this));
		return true;
	}

	void ServerUDP::Stop()
	{
		if (IsRunning())
		{
			m_Stop = true;
		}
	}

	bool ServerUDP::IsRunning() const
	{
		return m_pThread != nullptr;
	}

	const std::string& ServerUDP::GetAddress()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pServerSocket)
			return m_pServerSocket->GetAddress();
		return "";
	}

	uint16 ServerUDP::GetPort()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pServerSocket)
			return m_pServerSocket->GetPort();
		return 0;
	}

	uint8 ServerUDP::GetNrOfClients() const
	{
		return uint8();
	}

	void ServerUDP::Run(std::string address, uint16 port)
	{
		m_pServerSocket = CreateServerSocket(address, port);
		if (!m_pServerSocket)
		{
			LOG_ERROR("[ServerUDP]: Failed to start!");
			return;
		}

		LOG_INFO("[ServerUDP]: Started %s:%d", address.c_str(), port);

		NetworkPacket packet(PACKET_TYPE_UNDEFINED);
		int32 bytesReceived = 0;
		while (!m_Stop)
		{
			if (m_pServerSocket->ReceiveFrom(packet.GetBuffer(), MAXIMUM_PACKET_SIZE, bytesReceived, address, port))
			{
				if (bytesReceived > 0)
				{
					HandleReceivedPacket(&packet, address, port);
				}
			}
		}
	}

	void ServerUDP::OnStopped()
	{
	}

	void ServerUDP::HandleReceivedPacket(NetworkPacket* packet, const std::string& address, uint16 port)
	{
		packet->UnPack();
		// HASH address with port ?
	}

	ISocketUDP* ServerUDP::CreateServerSocket(const std::string& address, uint16 port)
	{
		ISocketUDP* serverSocket = PlatformSocketFactory::CreateSocketUDP();
		if (!serverSocket)
			return nullptr;

		if (!serverSocket->Bind(address, port))
		{
			serverSocket->Close();
			delete serverSocket;
			return nullptr;
		}

		return serverSocket;
	}
}