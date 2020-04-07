#include "Network/API/ClientUDP.h"
#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/NetworkPacket.h"

#include "Threading/Thread.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDP::ClientUDP(const std::string& address, uint16 port, IClientUDPHandler* handler) : 
		ClientBase(false),
		m_pSocket(nullptr),
		m_pClientHandler(handler)
	{
		SetAddressAndPort(address, port);
		StartThreads();
	}

	ClientUDP::~ClientUDP()
	{
		LOG_WARNING("~ClientUDP()");
	}

	void ClientUDP::OnTransmitterStarted()
	{
		m_pSocket = PlatformSocketFactory::CreateSocketUDP();
		if (!m_pSocket)
		{
			if (m_pClientHandler)
			{
				m_pClientHandler->OnClientErrorUDP(this);
			}

			TerminateThreads();
		}

		NetworkPacket packet(PACKET_TYPE_UNDEFINED, false);
		SendPacketImmediately(&packet);
	}

	void ClientUDP::OnReceiverStarted()
	{

	}

	void ClientUDP::UpdateReceiver(NetworkPacket* packet)
	{
		std::string address;
		uint16 port = 0;
		int32 bytesReceived = 0;

		while (!ShouldTerminate())
		{
			if (m_pSocket->ReceiveFrom(packet->GetBuffer(), MAXIMUM_PACKET_SIZE, bytesReceived, address, port))
			{
				packet->Reset();
				packet->UnPack();
				if (bytesReceived == packet->GetSize())
				{
					m_pClientHandler->OnClientPacketReceivedUDP(this, packet);
					LOG_MESSAGE("%s:%d", address.c_str(), port);
				}
				RegisterPacketsReceived(1);
			}
			else
			{
				TerminateThreads();
			}
		}
	}

	void ClientUDP::OnThreadsTerminated()
	{
		LOG_MESSAGE("OnThreadsTerminated()");
		std::scoped_lock<SpinLock> lock(m_LockStart);
		delete m_pSocket;
		m_pSocket = nullptr;

		if (m_pClientHandler)
		{
			m_pClientHandler->OnClientStoppedUDP(this);
			m_pClientHandler = nullptr;
		}
	}

	void ClientUDP::OnReleaseRequested()
	{
		if (m_pSocket)
		{
			if (!ThreadsHaveTerminated())
			{
				TerminateThreads();
				std::scoped_lock<SpinLock> lock(m_LockStart);
				m_pClientHandler = nullptr;
				if (m_pSocket)
					m_pSocket->Close();
			}
			else
			{
				std::scoped_lock<SpinLock> lock(m_LockStart);
				m_pClientHandler = nullptr;
			}
		}
	}

	bool ClientUDP::TransmitPacket(NetworkPacket* packet)
	{
		char* buffer = packet->GetBuffer();
		int32 bytesToSend = packet->GetSize();
		int32 bytesSent = 0;

		if (!m_pSocket->SendTo(buffer, bytesToSend, bytesSent, GetAddress(), GetPort()))
		{
			return false;
		}

		return bytesToSend == bytesSent;
	}
}