#include "Networking/API/IPAddress.h"
#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IClientHandler.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/SegmentPool.h"
#include "Networking/API/NetworkChallenge.h"

#include "Networking/API/UDP/ISocketUDP.h"
#include "Networking/API/UDP/ClientUDP.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ClientUDP::ClientUDP(const ClientDesc& desc) :
		ClientBase(desc),
		m_Transciver(),
		m_PacketManager(desc)
	{
		
	}

	ClientUDP::~ClientUDP()
	{
		LOG_INFO("[ClientUDP]: Released");
	}

	void ClientUDP::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateReceivingPacketLoss(lossRatio);
	}

	void ClientUDP::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_Transciver.SetSimulateTransmittingPacketLoss(lossRatio);
	}

	PacketManagerBase* ClientUDP::GetPacketManager()
	{
		return &m_PacketManager;
	}

	const PacketManagerBase* ClientUDP::GetPacketManager() const
	{
		return &m_PacketManager;
	}

	PacketTransceiverBase* ClientUDP::GetTransceiver()
	{
		return &m_Transciver;
	}

	ISocket* ClientUDP::SetupSocket(std::string& reason)
	{
		ISocketUDP* pSocket = PlatformNetworkUtils::CreateSocketUDP();
		if (pSocket)
		{
			IPEndPoint endPoint(IPAddress::ANY, 0);
			if (pSocket->Bind(endPoint))
			{
				return pSocket;
			}
			reason = "Bind Socket Failed " + endPoint.ToString();
			delete pSocket;
			return nullptr;
		}
		reason = "Create Socket Failed";
		return nullptr;
	}

	void ClientUDP::RunReceiver()
	{
		IPEndPoint sender;
		while (!ShouldTerminate())
		{
			if (!m_Transciver.ReceiveBegin(sender))
				continue;

			if (sender != GetEndPoint())
				continue;

			DecodeReceivedPackets();
		}
	}

	void ClientUDP::OnPacketDelivered(NetworkSegment* pPacket)
	{
		UNREFERENCED_VARIABLE(pPacket);
	}

	void ClientUDP::OnPacketResent(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_WARNING("ClientUDP::OnPacketResent(%d) | %s", tries, pPacket->ToString().c_str());
	}

	void ClientUDP::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries)
	{
		LOG_ERROR("ClientUDP::OnPacketMaxTriesReached(%d) | %s", tries, pPacket->ToString().c_str());
		Disconnect("Max Tries Reached");
	}
}