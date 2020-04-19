#include "Networking/API/ClientUDPRemote.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerUDP.h"
#include "Networking/API/IClientUDPHandler.h"
#include "Networking/API/BinaryDecoder.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDPRemote::ClientUDPRemote(uint16 packets, const IPEndPoint& ipEndPoint, ServerUDP* pServer) :
		m_pServer(pServer),
		m_IPEndPoint(ipEndPoint),
		m_PacketManager(packets),
		m_pHandler(nullptr),
		m_State(STATE_CONNECTING),
		m_pPackets()
	{
		m_PacketManager.GenerateSalt();
	}

	ClientUDPRemote::~ClientUDPRemote()
	{
		m_pHandler->OnDisconnectedUDP(this);
		LOG_INFO("[ClientUDPRemote]: Released");
	}

	PacketManager* ClientUDPRemote::GetPacketManager()
	{
		return &m_PacketManager;
	}

	void ClientUDPRemote::OnDataReceived(const char* data, int32 size)
	{
		int32 packetsReceived;
		if (m_PacketManager.DecodePackets(data, size, m_pPackets, packetsReceived))
		{
			for (int i = 0; i < packetsReceived; i++)
			{
				HandleReceivedPacket(m_pPackets[i]);
			}
			m_PacketManager.Free(m_pPackets, packetsReceived);
		}
	}

	void ClientUDPRemote::SendPackets(char* data)
	{
		int32 bytesWritten = 0;
		bool done = false;

		while (!done)
		{
			done = m_PacketManager.EncodePackets(data, bytesWritten);
			if (bytesWritten > 0)
			{
				m_pServer->Transmit(m_IPEndPoint, data, bytesWritten);
			}
		}
	}

	void ClientUDPRemote::HandleReceivedPacket(NetworkPacket* pPacket)
	{
		uint16 packetType = pPacket->GetType();

		if (packetType == NetworkPacket::TYPE_CONNNECT)
		{
			NetworkPacket* pResponse = GetFreePacket();
			pResponse->SetType(NetworkPacket::TYPE_CHALLENGE);
			m_PacketManager.EnqueuePacket(pResponse);

			if (!m_pHandler)
			{
				m_pHandler = m_pServer->CreateClientUDPHandler();
				m_pHandler->OnConnectingUDP(this);
			}
		}
		else if (packetType == NetworkPacket::TYPE_CHALLENGE)
		{
			uint64 answer = PacketManager::DoChallenge(m_PacketManager.GetSalt(), pPacket->GetRemoteSalt());
			
			BinaryDecoder decoder(pPacket);
			if (decoder.ReadUInt64() == answer)
			{
				NetworkPacket* pResponse = GetFreePacket();
				pResponse->SetType(NetworkPacket::TYPE_ACCEPTED);
				m_PacketManager.EnqueuePacket(pResponse);

				if (m_State == STATE_CONNECTING)
				{
					m_State = STATE_CONNECTED;
					m_pHandler->OnConnectedUDP(this);
				}
			}
			else
			{
				LOG_ERROR("[ClientUDPRemote]: Client responded with the wrong answer, is it a fake client? [%s]", m_IPEndPoint.ToString().c_str());
			}	
		}
		else if (packetType == NetworkPacket::TYPE_DISCONNECT)
		{
			Disconnect();
		}
		else if(IsConnected())
		{
			m_pHandler->OnPacketReceivedUDP(this, pPacket);
		}
	}

	bool ClientUDPRemote::Connect(const IPEndPoint& ipEndPoint)
	{
		return false;
	}

	void ClientUDPRemote::Disconnect()
	{
		m_pHandler->OnDisconnectingUDP(this);
	}

	bool ClientUDPRemote::IsConnected()
	{
		return m_State == STATE_CONNECTED;
	}

	bool ClientUDPRemote::SendUnreliable(NetworkPacket* packet)
	{
		return SendReliable(packet, nullptr);
	}

	bool ClientUDPRemote::SendReliable(NetworkPacket* packet, IPacketListener* listener)
	{
		if (!IsConnected())
		{
			LOG_WARNING("[ClientUDPRemote]: Can not send packet before a connection has been established");
			return false;
		}

		m_PacketManager.EnqueuePacket(packet, listener);
		return true;
	}

	const IPEndPoint& ClientUDPRemote::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	NetworkPacket* ClientUDPRemote::GetFreePacket()
	{
		return m_PacketManager.GetFreePacket();
	}

	EClientState ClientUDPRemote::GetState() const
	{
		return m_State;
	}
}