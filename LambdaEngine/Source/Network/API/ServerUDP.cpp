#include "Network/API/ServerUDP.h"
#include "Log/Log.h"
#include "Threading/Thread.h"
#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/NetworkPacket.h"

namespace LambdaEngine
{
	ServerUDP::ServerUDP(IServerUDPHandler* handler) : 
		m_pServerSocket(nullptr),
		m_pThread(nullptr),
		m_Stop(false),
		m_pHandler(handler)
	{
		UNREFERENCED_VARIABLE(handler);
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
		//Should return copy or reference (May cause crash in release)
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
				packet.Reset();
				packet.UnPack();
				if (bytesReceived != packet.GetSize())
				{
					m_pHandler->OnPacketReceivedUDP(&packet, address, port);
				}
			}
			else
			{
				Stop();
			}
		}
	}

	void ServerUDP::OnStopped()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pThread = nullptr;
		delete m_pServerSocket;
		m_pServerSocket = nullptr;
		LOG_WARNING("[ServerUDP]: Stopped");
	}

	/*void ServerUDP::HandleReceivedPacket(NetworkPacket* packet, const std::string& address, uint16 port)
	{
		UNREFERENCED_VARIABLE(packet);
		UNREFERENCED_VARIABLE(address);
		UNREFERENCED_VARIABLE(port);

		packet->UnPack();
		// HASH address with port ?
	}*/

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