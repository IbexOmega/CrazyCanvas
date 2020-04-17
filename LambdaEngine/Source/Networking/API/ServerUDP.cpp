#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerUDP.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/ClientUDPRemote.h"

#include "Log/Log.h"

#include "Networking/API/BinaryEncoder.h"

namespace LambdaEngine
{
	ServerUDP::ServerUDP(uint16 packetPerClient) : 
		m_pSocket(nullptr),
		m_Accepting(true),
		m_PacketsPerClient(packetPerClient)
	{

	}

	ServerUDP::~ServerUDP()
	{

	}

	bool ServerUDP::Start(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				m_IPEndPoint = ipEndPoint;
				LOG_WARNING("[ServerUDP]: Starting...");
				return true;
			}
		}
		return false;
	}

	void ServerUDP::Stop()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	bool ServerUDP::IsRunning()
	{
		return ThreadsAreRunning() && !ShouldTerminate();
	}

	const IPEndPoint& ServerUDP::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void ServerUDP::SetAcceptingConnections(bool accepting)
	{
		m_Accepting = accepting;
	}

	bool ServerUDP::IsAcceptingConnections()
	{
		return m_Accepting;
	}

	bool ServerUDP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(m_IPEndPoint))
			{
				LOG_INFO("[ServerUDP]: Started %s", m_IPEndPoint.ToString().c_str());
				return true;
			}
		}
		return false;
	}

	void ServerUDP::RunReceiver()
	{
		int32 bytesReceived = 0;
		int32 packetsReceived = 0;
		NetworkPacket* packets[32];
		IPEndPoint sender(IPAddress::NONE, 0);

		while (!ShouldTerminate())
		{
			if (!m_pSocket->ReceiveFrom(m_pReceiveBuffer, UINT16_MAX_, bytesReceived, sender))
			{
				TerminateThreads();
				break;
			}

			ClientUDPRemote* pClient = nullptr;
			auto pIterator = m_Clients.find(sender);
			if (pIterator != m_Clients.end())
			{
				pClient = pIterator->second;
			}
			else
			{
				pClient = DBG_NEW ClientUDPRemote(m_PacketsPerClient, sender, this);
				m_Clients.insert({ sender, pClient });
			}

			//pClient->

			//m_Accepting

			/*if (m_PacketManager.DecodePackets(m_pReceiveBuffer, bytesReceived, packets, packetsReceived))
			{
				for (int i = 0; i < packetsReceived; i++)
				{
					
					NetworkPacket* response = GetFreePacket();
					BinaryEncoder encoder(response);
					encoder.WriteString("I got your message");
					SendUnreliable(response);
					//OnPacketReceived(packets[i], sender);
				}
				m_PacketManager.Free(packets, packetsReceived);
			}*/
		}
	}

	void ServerUDP::RunTranmitter()
	{
		int32 bytesWritten = 0;
		int32 bytesSent = 0;
		bool done = false;

		while (!ShouldTerminate())
		{
			for (auto& tuple : m_Clients)
			{
				tuple.second->SendPackets(m_pSendBuffer);
			}
			YieldTransmitter();
		}
	}
	
	void ServerUDP::OnThreadsTurminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ServerUDP]: Stopped");
	}

	void ServerUDP::OnTerminationRequested()
	{
		LOG_WARNING("[ServerUDP]: Stopping...");
	}

	void ServerUDP::OnReleaseRequested()
	{
		Stop();
	}

	void ServerUDP::Transmit(const IPEndPoint& ipEndPoint, const char* data, int32 bytesToWrite)
	{
		int32 bytesSent = 0;
		if (!m_pSocket->SendTo(data, bytesToWrite, bytesSent, ipEndPoint))
		{
			TerminateThreads();
		}
		else if (bytesToWrite != bytesSent)
		{
			TerminateThreads();
		}
	}

	ServerUDP* ServerUDP::Create(uint16 packetsPerClient)
	{
		return DBG_NEW ServerUDP(packetsPerClient);
	}
}