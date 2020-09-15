#include "Networking/API/ClientBase.h"
#include "Networking/API/ISocket.h"
#include "Networking/API/IClientHandler.h"
#include "Networking/API/PacketManagerBase.h"
#include "Networking/API/PacketTransceiverBase.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/NetworkChallenge.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	std::set<ClientBase*> ClientBase::s_Clients;
	SpinLock ClientBase::s_Lock;

	ClientBase::ClientBase(const ClientDesc& desc) :
		m_pSocket(nullptr),
		m_pHandler(desc.Handler),
		m_PingTimeout(desc.PingTimeout),
		m_UsePingSystem(desc.UsePingSystem),
		m_LastPingTimestamp(0),
		m_State(STATE_DISCONNECTED),
		m_SendDisconnectPacket(false)
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Clients.insert(this);
	}

	ClientBase::~ClientBase()
	{
		std::scoped_lock<SpinLock> lock(s_Lock);
		s_Clients.erase(this);
	}

	bool ClientBase::Connect(const IPEndPoint& ipEndPoint)
	{
		if (!ThreadsAreRunning() && ThreadsHasTerminated())
		{
			GetPacketManager()->SetEndPoint(ipEndPoint);
			if (StartThreads())
			{
				LOG_WARNING("[ClientBase]: Connecting...");
				return true;
			}
		}
		return false;
	}

	void ClientBase::Disconnect(const std::string& reason)
	{
		TerminateThreads(reason);

		std::scoped_lock<SpinLock> lock(m_Lock);
		if (m_pSocket)
			m_pSocket->Close();
	}

	void ClientBase::Release()
	{
		NetWorker::TerminateAndRelease("Release Requested");
	}

	bool ClientBase::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	const IPEndPoint& ClientBase::GetEndPoint() const
	{
		return GetPacketManager()->GetEndPoint();
	}

	NetworkSegment* ClientBase::GetFreePacket(uint16 packetType)
	{
#ifdef LAMBDA_CONFIG_DEBUG
		return GetPacketManager()->GetSegmentPool()->RequestFreeSegment("ClientBase")->SetType(packetType);
#else
		return GetPacketManager()->GetSegmentPool()->RequestFreeSegment()->SetType(packetType);
#endif
	}

	EClientState ClientBase::GetState() const
	{
		return m_State;
	}

	const NetworkStatistics* ClientBase::GetStatistics() const
	{
		return GetPacketManager()->GetStatistics();
	}

	bool ClientBase::SendUnreliable(NetworkSegment* packet)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientBase]: Can not send packet before a connection has been established");
			return false;
		}

		GetPacketManager()->EnqueueSegmentUnreliable(packet);
		return true;
	}

	bool ClientBase::SendReliable(NetworkSegment* packet, IPacketListener* listener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientBase]: Can not send packet before a connection has been established");
			return false;
		}

		GetPacketManager()->EnqueueSegmentReliable(packet, listener);
		return true;
	}

	void ClientBase::SendConnect()
	{
		GetPacketManager()->EnqueueSegmentReliable(GetFreePacket(NetworkSegment::TYPE_CONNNECT), this);
		TransmitPackets();
	}

	void ClientBase::SendDisconnect()
	{
		GetPacketManager()->EnqueueSegmentReliable(GetFreePacket(NetworkSegment::TYPE_DISCONNECT), this);
		TransmitPackets();
	}

	void ClientBase::Tick(Timestamp delta)
	{
		if (m_State != STATE_DISCONNECTED)
		{
			GetPacketManager()->Tick(delta);
		}

		if (m_UsePingSystem)
		{
			UpdatePingSystem();
		}

		Flush();
	}

	void ClientBase::UpdatePingSystem()
	{
		if (m_State == STATE_CONNECTING || m_State == STATE_CONNECTED)
		{
			Timestamp timeSinceLastPacketReceived = EngineLoop::GetTimeSinceStart() - GetStatistics()->GetTimestampLastReceived();
			if (timeSinceLastPacketReceived >= m_PingTimeout)
			{
				Disconnect("Ping Timed Out");
			}

			if (m_State == STATE_CONNECTED)
			{
				Timestamp timeSinceLastPacketSent = EngineLoop::GetTimeSinceStart() - m_LastPingTimestamp;
				if (timeSinceLastPacketSent >= Timestamp::Seconds(1))
				{
					m_LastPingTimestamp = EngineLoop::GetTimeSinceStart();
					SendReliable(GetFreePacket(NetworkSegment::TYPE_PING));
				}
			}
		}
	}

	void ClientBase::TransmitPackets()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		GetPacketManager()->Flush(GetTransceiver());
	}

	void ClientBase::DecodeReceivedPackets()
	{
		TArray<NetworkSegment*> packets;
		PacketManagerBase* pPacketManager = GetPacketManager();
		pPacketManager->QueryBegin(GetTransceiver(), packets);
		for (NetworkSegment* pPacket : packets)
		{
			HandleReceivedPacket(pPacket);
		}
		pPacketManager->QueryEnd(packets);
	}

	void ClientBase::HandleReceivedPacket(NetworkSegment* pPacket)
	{
		uint16 packetType = pPacket->GetType();

		LOG_MESSAGE("ClientBase::HandleReceivedPacket(%s)", pPacket->ToString().c_str());

		if (packetType == NetworkSegment::TYPE_CHALLENGE)
		{
			uint64 answer = NetworkChallenge::Compute(GetStatistics()->GetSalt(), pPacket->GetRemoteSalt());
			ASSERT(answer != 0);

			NetworkSegment* pResponse = GetFreePacket(NetworkSegment::TYPE_CHALLENGE);
			BinaryEncoder encoder(pResponse);
			encoder.WriteUInt64(answer);
			GetPacketManager()->EnqueueSegmentReliable(pResponse, this);
		}
		else if (packetType == NetworkSegment::TYPE_ACCEPTED)
		{
			if (m_State == STATE_CONNECTING)
			{
				LOG_INFO("[ClientBase]: Connected");
				m_State = STATE_CONNECTED;
				m_pHandler->OnConnected(this);
			}
		}
		else if (packetType == NetworkSegment::TYPE_DISCONNECT)
		{
			m_SendDisconnectPacket = false;
			Disconnect("Disconnected By Remote");
		}
		else if (packetType == NetworkSegment::TYPE_SERVER_FULL)
		{
			m_SendDisconnectPacket = false;
			m_pHandler->OnServerFull(this);
			Disconnect("Server Full");
		}
		else if (packetType == NetworkSegment::TYPE_PING)
		{
			
		}
		else
		{
			m_pHandler->OnPacketReceived(this, pPacket);
		}
	}

	bool ClientBase::OnThreadsStarted(std::string& reason)
	{
		m_pSocket = SetupSocket(reason);
		if (m_pSocket)
		{
			GetTransceiver()->SetSocket(m_pSocket);
			GetPacketManager()->Reset();
			m_State = STATE_CONNECTING;
			m_pHandler->OnConnecting(this);
			m_SendDisconnectPacket = true;
			SendConnect();
			return true;
		}
		return false;
	}

	void ClientBase::RunTransmitter()
	{
		while (!ShouldTerminate())
		{
			TransmitPackets();
			YieldTransmitter();
		}
	}

	void ClientBase::OnThreadsTerminated()
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		if (m_pSocket)
		{
			m_pSocket->Close();
			delete m_pSocket;
			m_pSocket = nullptr;
		}
		
		LOG_INFO("[ClientBase]: Disconnected");
		m_State = STATE_DISCONNECTED;
		if (m_pHandler)
			m_pHandler->OnDisconnected(this);
	}

	void ClientBase::OnTerminationRequested(const std::string& reason)
	{
		LOG_WARNING("[ClientBase]: Disconnecting... [%s]", reason.c_str());
		m_State = STATE_DISCONNECTING;
		m_pHandler->OnDisconnecting(this);

		if (m_SendDisconnectPacket)
			SendDisconnect();
	}

	void ClientBase::OnReleaseRequested(const std::string& reason)
	{
		Disconnect(reason);
		m_pHandler->OnClientReleased(this);
		m_pHandler = nullptr;
	}

	void ClientBase::FixedTickStatic(Timestamp timestamp)
	{
		if (!s_Clients.empty())
		{
			std::scoped_lock<SpinLock> lock(s_Lock);
			for (ClientBase* client : s_Clients)
			{
				client->Tick(timestamp);
			}
		}
	}
}
