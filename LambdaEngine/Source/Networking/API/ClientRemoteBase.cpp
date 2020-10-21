#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/NetworkChallenge.h"
#include "Networking/API/ServerBase.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	ClientRemoteBase::ClientRemoteBase(const ClientRemoteDesc& desc) :
		m_pServer(desc.Server),
		m_PingInterval(desc.PingInterval),
		m_PingTimeout(desc.PingTimeout),
		m_UsePingSystem(desc.UsePingSystem),
		m_pHandler(nullptr),
		m_State(STATE_CONNECTING),
		m_LastPingTimestamp(0),
		m_DisconnectedByRemote(false),
		m_TerminationRequested(false),
		m_TerminationApproved(false),
		m_BufferIndex(0),
		m_ReceivedPackets(),
		m_LockReceivedPackets()
    {

    }

	ClientRemoteBase::~ClientRemoteBase()
	{
		if (!m_TerminationApproved)
			LOG_ERROR("[ClientRemoteBase]: Do not use delete on a ClientRemoteBase object. Use the Release() function!");
		else
			LOG_INFO("[ClientRemoteBase]: Released");
	}

	void ClientRemoteBase::Disconnect(const std::string& reason)
	{
		RequestTermination(reason);
	}

	bool ClientRemoteBase::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	void ClientRemoteBase::Release()
	{
		RequestTermination("Release Requested");
	}

	void ClientRemoteBase::ReleaseByServer()
	{
		RequestTermination("Release Requested By Server", true);
		OnTerminationApproved();
	}

	bool ClientRemoteBase::RequestTermination(const std::string& reason, bool byServer)
	{
		bool doRelease = false;
		bool enterCritical = false;
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			if (!m_TerminationRequested)
			{
				m_TerminationRequested = true;
				doRelease = true;
				enterCritical = OnTerminationRequested();
			}
		}

		if (doRelease)
		{
			if (enterCritical)
			{
				LOG_WARNING("[ClientRemoteBase]: Disconnecting... [%s]", reason.c_str());
				if (m_pHandler)
					m_pHandler->OnDisconnecting(this);

				if (!m_DisconnectedByRemote)
					SendDisconnect();

				if (!byServer)
					m_pServer->OnClientAskForTermination(this);

				m_State = STATE_DISCONNECTED;

				LOG_INFO("[ClientRemoteBase]: Disconnected");
				if (m_pHandler)
					m_pHandler->OnDisconnected(this);

				return true;
			}
		}
		return false;
	}

	bool ClientRemoteBase::OnTerminationRequested()
	{
		if (m_State == STATE_CONNECTING || m_State == STATE_CONNECTED)
		{
			m_State = STATE_DISCONNECTING;
			return true;
		}
		return false;
	}

	void ClientRemoteBase::OnTerminationApproved()
	{
		m_TerminationApproved = true;
		DeleteThis();
	}

	void ClientRemoteBase::DeleteThis()
	{
		if (CanDeleteNow())
		{
			if (m_pHandler)
				m_pHandler->OnClientReleased(this);
			delete this;
		}
	}

	bool ClientRemoteBase::CanDeleteNow()
	{
		return m_TerminationApproved;
	}

	void ClientRemoteBase::OnConnectionApproved()
	{
		m_pHandler->OnConnected(this);
	}

	bool ClientRemoteBase::SendUnreliable(NetworkSegment* pPacket)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientRemoteBase]: Can not send packet before a connection has been established");
			GetPacketManager()->GetSegmentPool()->FreeSegment(pPacket, "ClientRemoteBase::SendUnreliable");
			return false;
		}

		GetPacketManager()->EnqueueSegmentUnreliable(pPacket);
		return true;
	}

	bool ClientRemoteBase::SendReliable(NetworkSegment* pPacket, IPacketListener* pListener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientRemoteBase]: Can not send packet before a connection has been established");
			GetPacketManager()->GetSegmentPool()->FreeSegment(pPacket, "ClientRemoteBase::SendReliable");
			return false;
		}

		GetPacketManager()->EnqueueSegmentReliable(pPacket, pListener);
		return true;
	}

	bool ClientRemoteBase::SendReliableBroadcast(NetworkSegment* pPacket, IPacketListener* pListener)
	{
		return m_pServer->SendReliableBroadcast(this, pPacket, pListener);
	}

	bool ClientRemoteBase::SendUnreliableBroadcast(NetworkSegment* pPacket)
	{
		return m_pServer->SendUnreliableBroadcast(this, pPacket);
	}

	const ClientMap& ClientRemoteBase::GetClients() const
	{
		return m_pServer->GetClients();
	}

	ServerBase* ClientRemoteBase::GetServer()
	{
		return m_pServer;
	}

	const IPEndPoint& ClientRemoteBase::GetEndPoint() const
	{
		return GetPacketManager()->GetEndPoint();
	}

	NetworkSegment* ClientRemoteBase::GetFreePacket(uint16 packetType)
	{
#ifdef LAMBDA_CONFIG_DEBUG
		return GetPacketManager()->GetSegmentPool()->RequestFreeSegment("ClientRemoteBase")->SetType(packetType);
#else
		return GetPacketManager()->GetSegmentPool()->RequestFreeSegment()->SetType(packetType);
#endif
	}

	EClientState ClientRemoteBase::GetState() const
	{
		return m_State;
	}

	const NetworkStatistics* ClientRemoteBase::GetStatistics() const
	{
		return GetPacketManager()->GetStatistics();
	}

	IClientRemoteHandler* ClientRemoteBase::GetHandler()
	{
		return m_pHandler;
	}

	uint64 ClientRemoteBase::GetUID() const
	{
		return GetStatistics()->GetSalt();
	}

	void ClientRemoteBase::TransmitPackets()
	{
		GetPacketManager()->Flush(GetTransceiver());
	}

	void ClientRemoteBase::DecodeReceivedPackets()
	{
		if (m_State == STATE_CONNECTING || m_State == STATE_CONNECTED)
		{
			TArray<NetworkSegment*> packets;
			PacketManagerBase* pPacketManager = GetPacketManager();
			bool hasDiscardedResends = pPacketManager->QueryBegin(GetTransceiver(), packets);

			if (m_State == STATE_CONNECTING)
			{
				if (packets.IsEmpty() && !hasDiscardedResends)
				{
					Disconnect("Expected Connect Packets");
				}
				else
				{
					for (NetworkSegment* pPacket : packets)
					{
						if (pPacket->GetType() != NetworkSegment::TYPE_CONNNECT
							&& pPacket->GetType() != NetworkSegment::TYPE_CHALLENGE
							&& pPacket->GetType() != NetworkSegment::TYPE_DISCONNECT)
						{
							Disconnect("Expected Connect Packets");
							break;
						}
					}
				}
			}

			std::scoped_lock<SpinLock> lock(m_LockReceivedPackets);
			for (NetworkSegment* pPacket : packets)
			{
				if (!HandleReceivedPacket(pPacket))
				{
					pPacketManager->GetSegmentPool()->FreeSegment(pPacket, "ClientRemoteBase::DecodeReceivedPackets");
				}
			}
		}
	}

	void ClientRemoteBase::FixedTick(Timestamp delta)
	{
		GetPacketManager()->Tick(delta);

		if (m_UsePingSystem)
		{
			UpdatePingSystem();
		}

		HandleReceivedPacketsMainThread();
	}

	void ClientRemoteBase::HandleReceivedPacketsMainThread()
	{
		TArray<NetworkSegment*>& packets = m_ReceivedPackets[m_BufferIndex];

		{
			std::scoped_lock<SpinLock> lock(m_LockReceivedPackets);
			m_BufferIndex = (m_BufferIndex + 1) % 2;
		}

		for (NetworkSegment* pPacket : packets)
		{
			m_pHandler->OnPacketReceived(this, pPacket);
		}
		GetPacketManager()->QueryEnd(packets);
		packets.Clear();
	}

	void ClientRemoteBase::UpdatePingSystem()
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
				if (timeSinceLastPacketSent >= m_PingInterval)
				{
					m_LastPingTimestamp = EngineLoop::GetTimeSinceStart();
					NetworkSegment* pPacket = GetFreePacket(NetworkSegment::TYPE_PING);
					BinaryEncoder encoder(pPacket);
					NetworkStatistics& pStatistics = GetPacketManager()->m_Statistics;
					encoder.WriteUInt32(pStatistics.GetPacketsSent());
					encoder.WriteUInt32(pStatistics.GetPacketsReceived());
					pStatistics.UpdatePacketsSentFixed();
					SendReliable(pPacket);
				}
			}
		}
	}

	bool ClientRemoteBase::HandleReceivedPacket(NetworkSegment* pPacket)
	{
		uint16 packetType = pPacket->GetType();

		//LOG_MESSAGE("ClientRemoteBase::HandleReceivedPacket(%s)", pPacket->ToString().c_str());

		if (packetType == NetworkSegment::TYPE_CONNNECT)
		{
			GetPacketManager()->EnqueueSegmentUnreliable(GetFreePacket(NetworkSegment::TYPE_CHALLENGE));

			if (!m_pHandler)
			{
				m_pHandler = m_pServer->CreateClientHandler();
				m_pHandler->OnConnecting(this);
			}
		}
		else if (packetType == NetworkSegment::TYPE_CHALLENGE)
		{
			uint64 expectedAnswer = NetworkChallenge::Compute(GetStatistics()->GetSalt(), pPacket->GetRemoteSalt());
			BinaryDecoder decoder(pPacket);
			uint64 answer = decoder.ReadUInt64();
			if (answer == expectedAnswer)
			{
				GetPacketManager()->EnqueueSegmentUnreliable(GetFreePacket(NetworkSegment::TYPE_ACCEPTED));

				if (m_State == STATE_CONNECTING)
				{
					m_State = STATE_CONNECTED;
					m_LastPingTimestamp = EngineLoop::GetTimeSinceStart();
				}
			}
			else
			{
				LOG_ERROR("[ClientRemoteBase]: Client responded with %lu, expected %lu, is it a fake client? [%s]", answer, expectedAnswer, GetEndPoint().ToString().c_str());
			}
		}
		else if (packetType == NetworkSegment::TYPE_DISCONNECT)
		{
			m_DisconnectedByRemote = true;
			Disconnect("Disconnected By Remote");
		}
		else if (packetType == NetworkSegment::TYPE_PING)
		{
			BinaryDecoder decoder(pPacket);
			uint32 packetsSentByRemote		= decoder.ReadUInt32() + 1;
			uint32 packetsReceivedByRemote	= decoder.ReadUInt32();
			NetworkStatistics& statistics = GetPacketManager()->m_Statistics;
			statistics.SetPacketsSentByRemote(packetsSentByRemote);
			statistics.SetPacketsReceivedByRemote(packetsReceivedByRemote);
		}
		else if (IsConnected())
		{
			m_ReceivedPackets[m_BufferIndex].PushBack(pPacket);
			return true;
		}
		return false;
	}

	void ClientRemoteBase::SendDisconnect()
	{
		GetPacketManager()->EnqueueSegmentUnreliable(GetFreePacket(NetworkSegment::TYPE_DISCONNECT));
		TransmitPackets();
	}

	void ClientRemoteBase::SendServerFull()
	{
		GetPacketManager()->EnqueueSegmentUnreliable(GetFreePacket(NetworkSegment::TYPE_SERVER_FULL));
		TransmitPackets();
	}

	void ClientRemoteBase::SendServerNotAccepting()
	{
		GetPacketManager()->EnqueueSegmentUnreliable(GetFreePacket(NetworkSegment::TYPE_SERVER_NOT_ACCEPTING));
		TransmitPackets();
	}

	void ClientRemoteBase::OnPacketDelivered(NetworkSegment* pPacket)
	{
		LOG_INFO("ClientRemoteBase::OnPacketDelivered() | %s", pPacket->ToString().c_str());
	}

	void ClientRemoteBase::OnPacketResent(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientRemoteBase::OnPacketResent(%d) | %s", tries, pPacket->ToString().c_str());
	}

	void ClientRemoteBase::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientRemoteBase::OnPacketMaxTriesReached(%d) | %s", tries, pPacket->ToString().c_str());
		Disconnect("Max Tries Reached");
	}
}