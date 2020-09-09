#include "Networking/API/ClientUDPRemote.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerUDP.h"
#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/PacketTransceiverUDP.h"
#include "Networking/API/NetworkChallenge.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDPRemote::ClientUDPRemote(uint16 packetPoolSize, uint8 maximumTries, const IPEndPoint& ipEndPoint, ServerUDP* pServer) :
		m_pServer(pServer),
		m_PacketManager({ packetPoolSize, maximumTries })
	{
		m_PacketManager.SetEndPoint(ipEndPoint);
	}

	ClientUDPRemote::~ClientUDPRemote()
	{
		if(!m_Release)
			LOG_ERROR("[ClientUDPRemote]: Do not use delete on a ClientUDPRemote object. Use the Release() function!");
		else
			LOG_INFO("[ClientUDPRemote]: Released");
	}

	void ClientUDPRemote::OnPacketDelivered(NetworkSegment* pPacket)
	{
		LOG_INFO("ClientUDPRemote::OnPacketDelivered() | %s", pPacket->ToString().c_str());
	}

	void ClientUDPRemote::OnPacketResent(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientUDPRemote::OnPacketResent(%d) | %s", tries, pPacket->ToString().c_str());
	}

	void ClientUDPRemote::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_INFO("ClientUDPRemote::OnPacketMaxTriesReached(%d) | %s", tries, pPacket->ToString().c_str());
		Disconnect();
	}

	PacketManagerBase* ClientUDPRemote::GetPacketManager()
	{
		return &m_PacketManager;
	}

	void ClientUDPRemote::OnDataReceived(PacketTransceiverBase* pTransciver)
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

	void ClientUDPRemote::SendPackets(PacketTransceiverBase* pTransciver)
	{
		m_PacketManager.Flush(pTransciver);
	}

	bool ClientUDPRemote::HandleReceivedPacket(NetworkSegment* pPacket)
	{
		uint16 packetType = pPacket->GetType();

		LOG_MESSAGE("ClientUDPRemote::HandleReceivedPacket(%s)", pPacket->ToString().c_str());

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
				LOG_ERROR("[ClientUDPRemote]: Client responded with %lu, expected %lu, is it a fake client? [%s]", answer, expectedAnswer, GetEndPoint().ToString().c_str());
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

	void ClientUDPRemote::Tick(Timestamp delta)
	{
		m_PacketManager.Tick(delta);
	}

	void ClientUDPRemote::Disconnect()
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

	bool ClientUDPRemote::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	bool ClientUDPRemote::SendUnreliable(NetworkSegment* packet)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientUDPRemote]: Can not send packet before a connection has been established");
			return false;
		}

		m_PacketManager.EnqueueSegmentUnreliable(packet);
		return true;
	}

	bool ClientUDPRemote::SendReliable(NetworkSegment* packet, IPacketListener* listener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientUDPRemote]: Can not send packet before a connection has been established");
			return false;
		}

		m_PacketManager.EnqueueSegmentReliable(packet, listener);
		return true;
	}

	const IPEndPoint& ClientUDPRemote::GetEndPoint() const
	{
		return m_PacketManager.GetEndPoint();
	}

	NetworkSegment* ClientUDPRemote::GetFreePacket(uint16 packetType)
	{
		return m_PacketManager.GetSegmentPool()->RequestFreeSegment()->SetType(packetType);
	}

	EClientState ClientUDPRemote::GetState() const
	{
		return m_State;
	}

	const NetworkStatistics* ClientUDPRemote::GetStatistics() const
	{
		return m_PacketManager.GetStatistics();
	}

	void ClientUDPRemote::Release()
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