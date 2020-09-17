#include "Networking/API/IPAddress.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IClientHandler.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/SegmentPool.h"
#include "Networking/API/NetworkChallenge.h"

#include "Networking/API/TCP/ISocketTCP.h"
#include "Networking/API/TCP/ClientTCP.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientTCP::ClientTCP(const ClientDesc& desc) :
		ClientBase(desc),
		m_Transceiver(),
		m_PacketManager(desc)
	{
		
	}

	ClientTCP::~ClientTCP()
	{
		LOG_INFO("[ClientTCP]: Released");
	}

	PacketManagerBase* ClientTCP::GetPacketManager()
	{
		return &m_PacketManager;
	}

	const PacketManagerBase* ClientTCP::GetPacketManager() const
	{
		return &m_PacketManager;
	}

	PacketTransceiverBase* ClientTCP::GetTransceiver()
	{
		return &m_Transceiver;
	}

	ISocket* ClientTCP::SetupSocket(std::string& reason)
	{
		ISocketTCP* pSocket = PlatformNetworkUtils::CreateSocketTCP();
		if (pSocket)
		{
			pSocket->EnableNaglesAlgorithm(false);
			if (pSocket->Connect(GetEndPoint()))
			{
				return pSocket;
			}
			reason = "Connect Socket Failed " + GetEndPoint().ToString();
			delete pSocket;
			return nullptr;
		}
		reason = "Create Socket Failed";
		return nullptr;
	}

	void ClientTCP::RunReceiver()
	{
		IPEndPoint dummy;
		while (!ShouldTerminate())
		{
			if (!m_Transceiver.ReceiveBegin(dummy))
			{
				m_SendDisconnectPacket = false;
				Disconnect("Connection Lost");
				return;
			}

			DecodeReceivedPackets();
		}
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
		Disconnect("Max Tries Reached");
	}
}