#include "Network/API/UDP/ClientUDP.h"
#include "Network/API/PlatformNetworkUtils.h"
#include "Network/API/NetworkPacket.h"

#include "Threading/Thread.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDP::ClientUDP(IClientUDPHandler* clientHandler) :
		ClientBase(false),
		m_pSocket(nullptr),
		m_pClientHandler(clientHandler)
	{
		
	}

	ClientUDP::~ClientUDP()
	{
		LOG_WARNING("~ClientUDP()");
	}

	bool ClientUDP::Start(const std::string& address, uint16 port)
	{
		std::scoped_lock<SpinLock> lock(m_LockStart);
		if (ThreadsHaveTerminated())
		{
			SetAddressAndPort(address, port);

			if (StartThreads())
			{
				return true;
			}
		}
		return false;
	}

	bool ClientUDP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (!m_pSocket)
		{
			if (m_pClientHandler)
			{
				m_pClientHandler->OnClientErrorUDP(this);
			}
			return false;
		}

		if (GetAddress() == ADDRESS_BROADCAST)
		{
			return m_pSocket->EnableBroadcast();
		}
		return true;
	}

	bool ClientUDP::OnThreadsStartedPost()
	{
		NetworkPacket packet(PACKET_TYPE_UNDEFINED, false);
		if (!SendPacketImmediately(&packet))
		{
			LOG_ERROR("[ClientUDP]: Failed to send init packet!");
			return false;
		}
		return true;
	}

	void ClientUDP::UpdateReceiver(NetworkPacket* packet)
	{
		std::string address;
		uint16 port = 0;
		int32 bytesReceived = 0;

		while (!ShouldTerminate())
		{
			if (m_pSocket->ReceiveFrom(m_ReceiveBuffer, MAXIMUM_DATAGRAM_SIZE, bytesReceived, address, port))
			{
				memcpy(packet->GetBuffer(), m_ReceiveBuffer, sizeof(PACKET_SIZE));
				packet->Reset();
				packet->UnPack();
				if (bytesReceived == packet->GetSize())
				{
					memcpy(packet->GetBuffer(), m_ReceiveBuffer, bytesReceived);
					m_pClientHandler->OnClientPacketReceivedUDP(this, packet);
				}
				else
				{
					LOG_ERROR("Error received a packet with the wrong size %i | %i", bytesReceived, packet->GetSize());
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