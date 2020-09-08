#include "Networking/API/ClientTCPRemote.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketTCP.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerTCP.h"
#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/PacketTransceiverTCP.h"
#include "Networking/API/NetworkChallenge.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientTCPRemote::ClientTCPRemote(uint16 packetPoolSize, uint8 maximumTries, const IPEndPoint& ipEndPoint, ServerTCP* pServer) :
		m_pServer(pServer),
		m_PacketManager({ packetPoolSize, maximumTries }),
		m_pHandler(nullptr),
		m_State(STATE_CONNECTING),
		m_Release(false),
		m_DisconnectedByRemote(false)
	{
		m_PacketManager.SetEndPoint(ipEndPoint);
	}

	ClientTCPRemote::~ClientTCPRemote()
	{
		if(!m_Release)
			LOG_ERROR("[ClientTCPRemote]: Do not use delete on a ClientTCPRemote object. Use the Release() function!");
		else
			LOG_INFO("[ClientTCPRemote]: Released");
	}

	void ClientTCPRemote::OnPacketDelivered(NetworkSegment* pPacket)
	{
		LOG_INFO("ClientTCPRemote::OnPacketDelivered() | %s", pPacket->ToString().c_str());
	}

	void ClientTCPRemote::OnPacketResent(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientTCPRemote::OnPacketResent(%d) | %s", tries, pPacket->ToString().c_str());
	}

	void ClientTCPRemote::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientTCPRemote::OnPacketMaxTriesReached(%d) | %s", tries, pPacket->ToString().c_str());
		Disconnect();
	}

	PacketManagerBase* ClientTCPRemote::GetPacketManager()
	{
		return &m_PacketManager;
	}

	void ClientTCPRemote::OnDataReceived(PacketTransceiverUDP* pTransciver)
	{
		TArray<NetworkSegment*> packets;
		m_PacketManager.QueryBegin(pTransciver, packets);
		for (NetworkSegment* pPacket : packets)
		{
			if (!HandleReceivedPacket(pPacket))
				return;
		}
		m_PacketManager.QueryEnd(packets);
	}

	void ClientTCPRemote::SendPackets(PacketTransceiverUDP* pTransciver)
	{
		m_PacketManager.Flush(pTransciver);
	}

	bool ClientTCPRemote::HandleReceivedPacket(NetworkSegment* pPacket)
	{
		uint16 packetType = pPacket->GetType();

		LOG_MESSAGE("ClientTCPRemote::OnPacketReceivedUDP(%s)", pPacket->ToString().c_str());

		if (packetType == NetworkSegment::TYPE_CONNNECT)
		{
			m_PacketManager.EnqueueSegmentUnreliable(GetFreePacket(NetworkSegment::TYPE_CHALLENGE));

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
				m_PacketManager.EnqueueSegmentUnreliable(GetFreePacket(NetworkSegment::TYPE_ACCEPTED));

				if (m_State == STATE_CONNECTING)
				{
					m_State = STATE_CONNECTED;
					m_pHandler->OnConnected(this);
				}
			}
			else
			{
				LOG_ERROR("[ClientTCPRemote]: Client responded with %lu, expected %lu, is it a fake client? [%s]", answer, expectedAnswer, GetEndPoint().ToString().c_str());
			}	
		}
		else if (packetType == NetworkSegment::TYPE_DISCONNECT)
		{
			m_DisconnectedByRemote = true;
			Disconnect();
			return false;
		}
		else if(IsConnected())
		{
			m_pHandler->OnPacketReceived(this, pPacket);
		}
		return true;
	}

	void ClientTCPRemote::Tick(Timestamp delta)
	{
		m_PacketManager.Tick(delta);
	}

	void ClientTCPRemote::Disconnect()
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

			m_pServer->OnClientDisconnected(this, !m_DisconnectedByRemote);
			m_State = STATE_DISCONNECTED;

			if (m_pHandler)
				m_pHandler->OnDisconnected(this);
		}
	}

	bool ClientTCPRemote::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	bool ClientTCPRemote::SendUnreliable(NetworkSegment* packet)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientTCPRemote]: Can not send packet before a connection has been established");
			return false;
		}

		m_PacketManager.EnqueueSegmentUnreliable(packet);
		return true;
	}

	bool ClientTCPRemote::SendReliable(NetworkSegment* packet, IPacketListener* listener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientTCPRemote]: Can not send packet before a connection has been established");
			return false;
		}

		m_PacketManager.EnqueueSegmentReliable(packet, listener);
		return true;
	}

	const IPEndPoint& ClientTCPRemote::GetEndPoint() const
	{
		return m_PacketManager.GetEndPoint();
	}

	NetworkSegment* ClientTCPRemote::GetFreePacket(uint16 packetType)
	{
		return m_PacketManager.GetSegmentPool()->RequestFreeSegment()->SetType(packetType);
	}

	EClientState ClientTCPRemote::GetState() const
	{
		return m_State;
	}

	const NetworkStatistics* ClientTCPRemote::GetStatistics() const
	{
		return m_PacketManager.GetStatistics();
	}

	void ClientTCPRemote::Release()
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
			delete this;
		}
	}
}