#include "Networking/API/ClientUDP.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/PlatformNetworkUtils.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDP::ClientUDP(uint16 packets) :
		m_pSocket(nullptr),
		m_PacketManager(packets)
	{

	}

	ClientUDP::~ClientUDP()
	{

	}

	bool ClientUDP::Connect(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				m_IPEndPoint = ipEndPoint;
				LOG_WARNING("[ClientUDP]: Connecting...");
				return true;
			}
		}
		return false;
	}

	void ClientUDP::Disconnect()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	bool ClientUDP::IsConnected()
	{
		return ThreadsAreRunning() && !ShouldTerminate();
	}

	bool ClientUDP::SendUnreliable(NetworkPacket* packet)
	{
		return SendReliable(packet, nullptr);
	}

	bool ClientUDP::SendReliable(NetworkPacket* packet, IPacketListener* listener)
	{
		if (!IsConnected())
			return false;

		m_PacketManager.EnqueuePacket(packet, listener);
		return true;
	}

	const IPEndPoint& ClientUDP::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	NetworkPacket* ClientUDP::GetFreePacket()
	{
		return m_PacketManager.GetFreePacket();
	}

	bool ClientUDP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(IPEndPoint(IPAddress::ANY, 0)))
			{
				return true;
			}
		}
		return false;
	}

	void ClientUDP::RunReceiver()
	{
		int32 bytesReceived = 0;
		int32 packetsReceived = 0;
		NetworkPacket* packets[32];
		IPEndPoint sender(IPAddress::NONE, 0);

		while (!ShouldTerminate())
		{
			if (!m_pSocket->ReceiveFrom(m_pReceiveBuffer, UINT16_MAX, bytesReceived, sender))
			{
				TerminateThreads();
				break;
			}

			if (m_PacketManager.DecodePackets(m_pReceiveBuffer, bytesReceived, packets, packetsReceived))
			{
				for (int i = 0; i < packetsReceived; i++)
				{
					//OnPacketReceived(packets[i], sender);
				}
				m_PacketManager.Free(packets, packetsReceived);
			}
		}
	}

	void ClientUDP::RunTranmitter()
	{
		int32 bytesWritten = 0;
		int32 bytesSent = 0;
		bool done = false;

		while (!ShouldTerminate())
		{
			while (!done)
			{
				done = m_PacketManager.EncodePackets(m_pSendBuffer, bytesWritten);
				if (bytesWritten > 0)
				{
					if (!m_pSocket->SendTo(m_pSendBuffer, bytesWritten, bytesSent, m_IPEndPoint))
					{
						TerminateThreads();
					}
				}
			}
			done = false;
			YieldTransmitter();
		}
	}

	void ClientUDP::OnThreadsTurminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ClientUDP]: Disconnected");
	}

	void ClientUDP::OnTerminationRequested()
	{
		LOG_WARNING("[ClientUDP]: Disconnecting...");
	}

	void ClientUDP::OnReleaseRequested()
	{
		Disconnect();
	}

	ClientUDP* ClientUDP::Create(uint16 packets)
	{
		return DBG_NEW ClientUDP(packets);
	}
}