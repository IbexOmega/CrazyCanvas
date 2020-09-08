#include "Networking/API/ClientTCP.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IClientHandler.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/SegmentPool.h"
#include "Networking/API/NetworkChallenge.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	std::set<ClientTCP*> ClientTCP::s_Clients;
	SpinLock ClientTCP::s_Lock;

	ClientTCP::ClientTCP(const ClientTCPDesc& desc) :
		m_pSocket(nullptr),
		m_PacketManager(desc),
		m_pHandler(desc.Handler),
		m_State(STATE_DISCONNECTED),
		m_pSendBuffer()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Clients.insert(this);
	}

	ClientTCP::~ClientTCP()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Clients.erase(this);
		LOG_INFO("[ClientTCP]: Released");
	}

	void ClientTCP::OnPacketDelivered(NetworkSegment* pPacket)
	{
		LOG_INFO("ClientTCP::OnPacketDelivered() | %s", pPacket->ToString().c_str());
	}

	void ClientTCP::OnPacketResent(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientTCP::OnPacketResent(%d) | %s", tries, pPacket->ToString().c_str());
	}

	void ClientTCP::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientTCP::OnPacketMaxTriesReached(%d) | %s", tries, pPacket->ToString().c_str());
		Disconnect();
	}

	bool ClientTCP::Connect(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				LOG_WARNING("[ClientTCP]: Connecting...");
				m_PacketManager.SetEndPoint(ipEndPoint);
				return true;
			}
		}
		return false;
	}

	void ClientTCP::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateReceivingPacketLoss(lossRatio);
	}

	void ClientTCP::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateTransmittingPacketLoss(lossRatio);
	}

	void ClientTCP::Disconnect()
	{
		TerminateThreads();

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ClientTCP::Release()
	{
		NetWorker::TerminateAndRelease();
	}

	bool ClientTCP::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	bool ClientTCP::SendUnreliable(NetworkSegment* packet)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientTCP]: Can not send packet before a connection has been established");
			return false;
		}

		m_PacketManager.EnqueueSegmentUnreliable(packet);
		return true;
	}

	bool ClientTCP::SendReliable(NetworkSegment* packet, IPacketListener* listener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientTCP]: Can not send packet before a connection has been established");
			return false;
		}
			
		m_PacketManager.EnqueueSegmentReliable(packet, listener);
		return true;
	}

	const IPEndPoint& ClientTCP::GetEndPoint() const
	{
		return m_PacketManager.GetEndPoint();
	}

	NetworkSegment* ClientTCP::GetFreePacket(uint16 packetType)
	{
		return m_PacketManager.GetSegmentPool()->RequestFreeSegment()->SetType(packetType);
	}

	EClientState ClientTCP::GetState() const
	{
		return m_State;
	}

	const NetworkStatistics* ClientTCP::GetStatistics() const
	{
		return m_PacketManager.GetStatistics();
	}

	PacketManagerUDP* ClientTCP::GetPacketManager()
	{
		return &m_PacketManager;
	}

	bool ClientTCP::OnThreadsStarted()
	{
		m_pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (m_pSocket)
		{
			if (m_pSocket->Bind(IPEndPoint(IPAddress::ANY, 0)))
			{
				m_Transciver.SetSocket(m_pSocket);
				m_PacketManager.Reset();
				m_State = STATE_CONNECTING;
				m_pHandler->OnConnecting(this);
				m_SendDisconnectPacket = true;
				SendConnectRequest();
				return true;
			}
			LOG_ERROR("[ClientTCP]: Failed To Bind socket");
			return false;
		}
		LOG_ERROR("[ClientTCP]: Failed To Create socket");
		return false;
	}

	void ClientTCP::RunReceiver()
	{
		IPEndPoint sender;
		while (!ShouldTerminate())
		{
			if (!m_Transciver.ReceiveBegin(sender))
				continue;

			TArray<NetworkSegment*> packets;
			m_PacketManager.QueryBegin(&m_Transciver, packets);
			for (NetworkSegment* pPacket : packets)
			{
				HandleReceivedPacket(pPacket);
			}
			m_PacketManager.QueryEnd(packets);
		}
	}

	void ClientTCP::RunTransmitter()
	{
		while (!ShouldTerminate())
		{
			TransmitPackets();
			YieldTransmitter();
		}
	}

	void ClientTCP::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket = nullptr;
		LOG_INFO("[ClientTCP]: Disconnected");
		m_State = STATE_DISCONNECTED;
		if(m_pHandler)
			m_pHandler->OnDisconnected(this);
	}

	void ClientTCP::OnTerminationRequested()
	{
		LOG_WARNING("[ClientTCP]: Disconnecting...");
		m_State = STATE_DISCONNECTING;
		m_pHandler->OnDisconnecting(this);

		if(m_SendDisconnectPacket)
			SendDisconnectRequest();
	}

	void ClientTCP::OnReleaseRequested()
	{
		Disconnect();
		m_pHandler = nullptr;
	}

	void ClientTCP::SendConnectRequest()
	{
		m_PacketManager.EnqueueSegmentReliable(GetFreePacket(NetworkSegment::TYPE_CONNNECT), this);
		TransmitPackets();
	}

	void ClientTCP::SendDisconnectRequest()
	{
		m_PacketManager.EnqueueSegmentReliable(GetFreePacket(NetworkSegment::TYPE_DISCONNECT), this);
		TransmitPackets();
	}

	void ClientTCP::HandleReceivedPacket(NetworkSegment* pPacket)
	{
		uint16 packetType = pPacket->GetType();

		if (packetType == NetworkSegment::TYPE_CHALLENGE)
		{
			uint64 answer = NetworkChallenge::Compute(GetStatistics()->GetSalt(), pPacket->GetRemoteSalt());
			ASSERT(answer != 0);

			NetworkSegment* pResponse = GetFreePacket(NetworkSegment::TYPE_CHALLENGE);
			BinaryEncoder encoder(pResponse);
			encoder.WriteUInt64(answer);
			m_PacketManager.EnqueueSegmentReliable(pResponse, this);
		}
		else if (packetType == NetworkSegment::TYPE_ACCEPTED)
		{
			if (m_State == STATE_CONNECTING)
			{
				LOG_INFO("[ClientTCP]: Connected");
				m_State = STATE_CONNECTED;
				m_pHandler->OnConnected(this);
			}
		}
		else if (packetType == NetworkSegment::TYPE_DISCONNECT)
		{
			m_SendDisconnectPacket = false;
			Disconnect();
		}
		else if (packetType == NetworkSegment::TYPE_SERVER_FULL)
		{
			m_SendDisconnectPacket = false;
			m_pHandler->OnServerFull(this);
			Disconnect();
		}
		else
		{
			m_pHandler->OnPacketReceived(this, pPacket);
		}
	}

	void ClientTCP::TransmitPackets()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		m_PacketManager.Flush(&m_Transciver);
	}

	void ClientTCP::Tick(Timestamp delta)
	{
		if (m_State != STATE_DISCONNECTED)
		{
			m_PacketManager.Tick(delta);
		}
		
		Flush();
	}

	ClientTCP* ClientTCP::Create(const ClientTCPDesc& desc)
	{
		return DBG_NEW ClientTCP(desc);
	}

	void ClientTCP::FixedTickStatic(Timestamp timestamp)
	{
		UNREFERENCED_VARIABLE(timestamp);

		if (!s_Clients.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (ClientTCP* client : s_Clients)
			{
				client->Tick(timestamp);
			}
		}
	}
}