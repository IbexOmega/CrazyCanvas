#include "Networking/API/ClientUDPRemote.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/ServerUDP.h"
#include "Networking/API/IClientUDPHandler.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDPRemote::ClientUDPRemote(uint16 packets, const IPEndPoint& ipEndPoint, IClientUDPHandler* pHandler, ServerUDP* pServer) :
		m_pServer(pServer),
		m_IPEndPoint(ipEndPoint),
		m_PacketManager(packets),
		m_pHandler(pHandler)
	{

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
				m_pHandler->OnPacketUDPReceived(this, m_pPackets[i]);
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

	ClientUDPRemote::~ClientUDPRemote()
	{

	}

	bool ClientUDPRemote::Connect(const IPEndPoint& ipEndPoint)
	{
		/*if (!ThreadsAreRunning())
		{
			if (StartThreads())
			{
				m_IPEndPoint = ipEndPoint;
				LOG_WARNING("[ClientUDP]: Connecting...");
				return true;
			}
		}*/
		return false;
	}

	void ClientUDPRemote::Disconnect()
	{
		
	}

	bool ClientUDPRemote::IsConnected()
	{
		return true;//ThreadsAreRunning() && !ShouldTerminate();
	}

	bool ClientUDPRemote::SendUnreliable(NetworkPacket* packet)
	{
		return SendReliable(packet, nullptr);
	}

	bool ClientUDPRemote::SendReliable(NetworkPacket* packet, IPacketListener* listener)
	{
		if (!IsConnected())
			return false;

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
}