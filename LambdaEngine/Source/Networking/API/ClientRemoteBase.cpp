#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
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
		m_TerminationApproved(false)
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
				if (m_pHandler)
				{
					LOG_WARNING("[ClientRemoteBase]: Disconnecting... [%s]", reason.c_str());
					m_pHandler->OnDisconnecting(this);
				}
					

				if (!m_DisconnectedByRemote)
					SendDisconnect();

				if (!byServer)
					m_pServer->OnClientAskForTermination(this);

				m_State = STATE_DISCONNECTED;

				if (m_pHandler)
				{
					LOG_INFO("[ClientRemoteBase]: Disconnected");
					m_pHandler->OnDisconnected(this);
				}

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

	bool ClientRemoteBase::SendUnreliable(NetworkSegment* packet)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientRemoteBase]: Can not send packet before a connection has been established");
			return false;
		}

		GetPacketManager()->EnqueueSegmentUnreliable(packet);
		return true;
	}

	bool ClientRemoteBase::SendReliable(NetworkSegment* packet, IPacketListener* listener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientRemoteBase]: Can not send packet before a connection has been established");
			return false;
		}

		GetPacketManager()->EnqueueSegmentReliable(packet, listener);
		return true;
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

	void ClientRemoteBase::TransmitPackets()
	{
		GetPacketManager()->Flush(GetTransceiver());
	}

	void ClientRemoteBase::DecodeReceivedPackets()
	{
		TArray<NetworkSegment*> packets;
		PacketManagerBase* pPacketManager = GetPacketManager();
		pPacketManager->QueryBegin(GetTransceiver(), packets);
		for (NetworkSegment* pPacket : packets)
		{
			if (!HandleReceivedPacket(pPacket))
				return;
		}
		pPacketManager->QueryEnd(packets);
	}

	void ClientRemoteBase::Tick(Timestamp delta)
	{
		GetPacketManager()->Tick(delta);

		if (m_UsePingSystem)
		{
			UpdatePingSystem();
		}	
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
					SendReliable(GetFreePacket(NetworkSegment::TYPE_PING));
				}
			}
		}
	}

	bool ClientRemoteBase::HandleReceivedPacket(NetworkSegment* pPacket)
	{
		uint16 packetType = pPacket->GetType();

		LOG_MESSAGE("ClientRemoteBase::HandleReceivedPacket(%s)", pPacket->ToString().c_str());

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
					m_pHandler->OnConnected(this);
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
			return false;
		}
		else if (IsConnected())
		{
			m_pHandler->OnPacketReceived(this, pPacket);
		}
		return true;
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