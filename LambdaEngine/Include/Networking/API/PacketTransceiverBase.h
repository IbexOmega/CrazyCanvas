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
	class ISocket;
	class SegmentPool;

	class LAMBDA_API PacketTransceiverBase
	{
	public:
		DECL_ABSTRACT_CLASS_NO_DEFAULT(PacketTransceiverBase);

		int32 Transmit(SegmentPool* pSegmentPool, std::queue<NetworkSegment*>& packets, std::set<uint32>& reliableUIDsSent, const IPEndPoint& endPoint, NetworkStatistics* pStatistics);
		bool ReceiveBegin(IPEndPoint& sender);
		bool ReceiveEnd(SegmentPool* pSegmentPool, TArray<NetworkSegment*>& packets, TArray<uint32>& newAcks, NetworkStatistics* pStatistics);

		virtual void SetSocket(ISocket* pSocket) = 0;

	protected:
		virtual bool TransmitData(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& endPoint) = 0;
		virtual bool ReceiveData(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& endPoint) = 0;
		virtual void OnReceiveEnd(PacketTranscoder::Header* pHeader, TArray<uint32>& newAcks, NetworkStatistics* pStatistics) = 0;

	private:
		static bool ValidateHeaderSalt(PacketTranscoder::Header* header, NetworkStatistics* pStatistics);

	private:
		int32 m_BytesReceived;
		uint8 m_pSendBuffer[MAXIMUM_SEGMENT_SIZE];
		uint8 m_pReceiveBuffer[UINT16_MAX];
	};
}