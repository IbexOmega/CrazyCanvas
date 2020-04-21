#include "Networking/API/ClientUDP.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IClientUDPHandler.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkStatistics.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDP::ClientUDP(IClientUDPHandler* pHandler, uint16 packets, uint8 maximumTries) :
		m_pSocket(nullptr),
		m_PacketManager(packets, maximumTries),
		m_pHandler(pHandler), 
		m_State(STATE_DISCONNECTED)
	{

	}

	void ClientUDP::OnPacketDelivered(NetworkPacket* pPacket)
	{
		LOG_INFO("OnPacketDelivered() | %s", pPacket->ToString().c_str());
	}

	void ClientUDP::OnPacketResent(NetworkPacket* pPacket, uint8 tries)
	{
		LOG_INFO("OnPacketResent(%d) | %s", tries, pPacket->ToString().c_str());
	}

	void ClientUDP::OnPacketMaxTriesReached(NetworkPacket* pPacket, uint8 tries)
	{
		LOG_INFO("OnPacketMaxTriesReached(%d) | %s", tries, pPacket->ToString().c_str());
	}

	ClientUDP::~ClientUDP()
	{
		LOG_INFO("[ClientUDP]: Released");
	}

	bool ClientUDP::Connect(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				LOG_WARNING("[ClientUDP]: Connecting...");
				m_IPEndPoint = ipEndPoint;
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

	void ClientUDP::Release()
	{
		NetWorker::TerminateAndRelease();
	}

	bool ClientUDP::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	bool ClientUDP::SendUnreliable(NetworkPacket* packet)
	{
		return SendReliable(packet, nullptr);
	}

	bool ClientUDP::SendReliable(NetworkPacket* packet, IPacketListener* listener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientUDP]: Can not send packet before a connection has been established");
			return false;
		}
			
		m_PacketManager.EnqueuePacket(packet, listener);
		return true;
	}

	const IPEndPoint& ClientUDP::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	NetworkPacket* ClientUDP::GetFreePacket(uint16 packetType)
	{
		return m_PacketManager.GetFreePacket()->SetType(packetType);
	}

	EClientState ClientUDP::GetState() const
	{
		return m_State;
	}

	const NetworkStatistics* ClientUDP::GetStatistics() const
	{
		return m_PacketManager.GetStatistics();
	}

	bool ClientUDP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(IPEndPoint(IPAddress::ANY, 0)))
			{
				m_State = STATE_CONNECTING;
				m_pHandler->OnConnectingUDP(this);
				m_PacketManager.Reset();
				SendConnectRequest();
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
		IPEndPoint sender;

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
					HandleReceivedPacket(packets[i]);
				}
				m_PacketManager.Free(packets, packetsReceived);
			}
		}
	}

	void ClientUDP::RunTranmitter()
	{
		while (!ShouldTerminate())
		{
			TransmitPackets();
			YieldTransmitter();
		}
	}

	void ClientUDP::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ClientUDP]: Disconnected");
		m_State = STATE_DISCONNECTED;
		if(m_pHandler)
			m_pHandler->OnDisconnectedUDP(this);
	}

	void ClientUDP::OnTerminationRequested()
	{
		LOG_WARNING("[ClientUDP]: Disconnecting...");
		m_State = STATE_DISCONNECTING;
		m_pHandler->OnDisconnectingUDP(this);
		SendDisconnectRequest();
	}

	void ClientUDP::OnReleaseRequested()
	{
		Disconnect();
		m_pHandler = nullptr;
	}

	void ClientUDP::SendConnectRequest()
	{
		m_PacketManager.EnqueuePacket(GetFreePacket(NetworkPacket::TYPE_CONNNECT), this);
		TransmitPackets();
	}

	void ClientUDP::SendDisconnectRequest()
	{
		m_PacketManager.EnqueuePacket(GetFreePacket(NetworkPacket::TYPE_DISCONNECT), this);
		TransmitPackets();
	}

	void ClientUDP::HandleReceivedPacket(NetworkPacket* pPacket)
	{
		LOG_MESSAGE("PING %fms", GetStatistics()->GetPing().AsMilliSeconds());
		uint16 packetType = pPacket->GetType();

		if (packetType == NetworkPacket::TYPE_CHALLENGE)
		{
			uint64 answer = PacketManager::DoChallenge(GetStatistics()->GetSalt(), pPacket->GetRemoteSalt());
			NetworkPacket* pResponse = GetFreePacket(NetworkPacket::TYPE_CHALLENGE);
			BinaryEncoder encoder(pResponse);
			encoder.WriteUInt64(answer);
			m_PacketManager.EnqueuePacket(pResponse, this);
		}
		else if (packetType == NetworkPacket::TYPE_ACCEPTED)
		{
			if (m_State == STATE_CONNECTING)
			{
				LOG_INFO("[ClientUDP]: Connected");
				m_State = STATE_CONNECTED;
				m_pHandler->OnConnectedUDP(this);
			}
		}
		else if (packetType == NetworkPacket::TYPE_DISCONNECT)
		{
			Disconnect();
		}
		else
		{
			m_pHandler->OnPacketReceivedUDP(this, pPacket);
		}
	}

	void ClientUDP::TransmitPackets()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		int32 bytesWritten = 0;
		int32 bytesSent = 0;
		bool done = false;

		m_PacketManager.Tick();
		m_PacketManager.SwapPacketQueues();

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
	}

	ClientUDP* ClientUDP::Create(IClientUDPHandler* pHandler, uint16 packets, uint8 maximumTries)
	{
		return DBG_NEW ClientUDP(pHandler, packets, maximumTries);
	}
}