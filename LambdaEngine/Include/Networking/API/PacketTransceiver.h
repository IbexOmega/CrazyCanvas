#pragma once

#include "LambdaEngine.h"
#include "Containers/TQueue.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/PacketTranscoder.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class NetworkStatistics;
	class ISocketUDP;

	class LAMBDA_API PacketTransceiver
	{
	public:
		PacketTransceiver();
		~PacketTransceiver();

		int32 Transmit(SegmentPool* pSegmentPool, std::queue<NetworkSegment*>& packets, std::set<uint32>& reliableUIDsSent, const IPEndPoint& ipEndPoint, NetworkStatistics* pStatistics);
		bool ReceiveBegin(IPEndPoint& sender);
		bool ReceiveEnd(SegmentPool* pSegmentPool, TArray<NetworkSegment*>& packets, TArray<uint32>& newAcks, NetworkStatistics* pStatistics);

		void SetSocket(ISocketUDP* pSocket);

		void SetSimulateReceivingPacketLoss(float32 lossRatio);
		void SetSimulateTransmittingPacketLoss(float32 lossRatio);

	private:
		static bool ValidateHeaderSalt(PacketTranscoder::Header* header, NetworkStatistics* pStatistics);
		static void ProcessSequence(uint32 sequence, NetworkStatistics* pStatistics);
		static void ProcessAcks(uint32 ack, uint32 ackBits, NetworkStatistics* pStatistics, TArray<uint32>& newAcks);

	private:
		ISocketUDP* m_pSocket;
		int32 m_BytesReceived;
		float32 m_ReceivingLossRatio;
		float32 m_TransmittingLossRatio;
		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
		char m_pReceiveBuffer[UINT16_MAX];
	};
}