#pragma once

#include "LambdaEngine.h"
#include "Containers/TQueue.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include "Containers/THashTable.h"

#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/SegmentPool.h"
#include "Networking/API/IPEndPoint.h"

#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class IPacketListener;
	class PacketTransceiver;

	struct PacketManagerDesc
	{
		uint16 PoolSize = 10;
		int32 MaxRetries = 10;
		float32 ResendRTTMultiplier = 2.0f;
	};

	class LAMBDA_API PacketManager
	{
	public:
		struct SegmentInfo
		{
			NetworkSegment* Packet		= nullptr;
			IPacketListener* Listener	= nullptr;
			Timestamp LastSent			= 0;
			uint8 Retries				= 0;
		};

		struct Bundle
		{
			std::set<uint32> ReliableUIDs;
			Timestamp Timestamp = 0;
		};

	public:
		PacketManager(const PacketManagerDesc& desc);
		~PacketManager();

		uint32 EnqueueSegmentReliable(NetworkSegment* pPacket, IPacketListener* pListener = nullptr);
		uint32 EnqueueSegmentUnreliable(NetworkSegment* pPacket);

		void Flush(PacketTransceiver* pTransceiver);
		void QueryBegin(PacketTransceiver* pTransceiver, TArray<NetworkSegment*>& packetsReturned);
		void QueryEnd(TArray<NetworkSegment*>& packetsReceived);

		void Tick(Timestamp delta);

		SegmentPool* GetSegmentPool();
		const NetworkStatistics* GetStatistics() const;
		const IPEndPoint& GetEndPoint() const;

		void SetEndPoint(const IPEndPoint& ipEndPoint);

		void Reset();

	private:
		uint32 EnqueueSegment(NetworkSegment* pSegment, uint32 reliableUID);
		void FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned);
		void UntangleReliableSegments(TArray<NetworkSegment*>& segmentsReturned);
		void HandleAcks(const TArray<uint32>& acks);
		void GetReliableUIDsFromAcks(const TArray<uint32>& acks, TArray<uint32>& ackedReliableUIDs);
		void GetReliableSegmentInfosFromUIDs(const TArray<uint32>& ackedReliableUIDs, TArray<SegmentInfo>& ackedReliableSegments);
		void RegisterRTT(Timestamp rtt);
		void DeleteOldBundles();
		void ResendOrDeleteSegments();

	private:
		NetworkStatistics m_Statistics;
		SegmentPool m_SegmentPool;
		IPEndPoint m_IPEndPoint;
		std::queue<NetworkSegment*> m_SegmentsToSend[2];
		std::unordered_map<uint32, SegmentInfo> m_SegmentsWaitingForAck;
		std::set<NetworkSegment*> m_ReliableSegmentsReceived;
		std::unordered_map<uint32, Bundle> m_Bundles;
		std::atomic_int m_QueueIndex;
		Timestamp m_Timer;
		float32 m_ResendRTTMultiplier;
		int32 m_MaxRetries;
		SpinLock m_LockSegmentsToSend;
		SpinLock m_LockBundles;
	};
}
