#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkChallenge.h"
#include "Networking/API/ServerBase.h"

namespace LambdaEngine
{
    ClientRemoteBase::ClientRemoteBase(ServerBase* pServer) :
		m_pServer(pServer),
		m_pHandler(nullptr),
		m_State(STATE_CONNECTING),
		m_Release(false),
		m_ReleasedByServer(false),
		m_DisconnectedByRemote(false)
    {

    }

	ClientRemoteBase::~ClientRemoteBase()
	{
		if (!m_Release)
			LOG_ERROR("[ClientRemoteBase]: Do not use delete on a ClientRemoteBase object. Use the Release() function!");
		else
			LOG_INFO("[ClientRemoteBase]: Released");
	}

	void ClientRemoteBase::Disconnect()
	{
		bool enterCritical = false;
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			if (m_State == STATE_CONNECTING || m_State == STATE_CONNECTED)
			{
				m_State = STATE_DISCONNECTING;
				enterCritical = true;
			}
		}

		if (enterCritical)
		{
			if (m_pHandler)
				m_pHandler->OnDisconnecting(this);

			if (!m_DisconnectedByRemote)
				SendDisconnect();

			if(!m_ReleasedByServer)
				m_pServer->OnClientDisconnected(this);

			m_State = STATE_DISCONNECTED;

			if (m_pHandler)
				m_pHandler->OnDisconnected(this);
		}
	}

	bool ClientRemoteBase::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	void ClientRemoteBase::Release()
	{
		bool doRelease = false;
		{
			std::scoped_lock<SpinLock> lock(m_Lock);
			if (!m_Release)
			{
				m_Release = true;
				doRelease = true;
			}
		}

		if (doRelease)
		{
			Disconnect();
			m_pHandler->OnClientReleased(this);
			delete this;
		}
	}

	void ClientRemoteBase::ReleaseByServer()
	{
		m_ReleasedByServer = true;
		Release();
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
		return GetPacketManager()->GetSegmentPool()->RequestFreeSegment()->SetType(packetType);
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
			Release();
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
		Disconnect();
	}
}