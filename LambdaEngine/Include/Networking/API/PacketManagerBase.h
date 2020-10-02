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
	class PacketTransceiverBase;

	struct PacketManagerDesc
	{
		uint16 PoolSize = 10;
		uint8 MaxRetries = 10;
		float32 ResendRTTMultiplier = 2.0f;
	};

	class LAMBDA_API PacketManagerBase
	{
		friend class ClientBase;
		friend class ClientRemoteBase;

	public:
		DECL_ABSTRACT_CLASS(PacketManagerBase);

		struct SegmentInfo
		{
			NetworkSegment* Segment = nullptr;
			IPacketListener* Listener = nullptr;
			Timestamp LastSent = 0;
			uint8 Retries = 0;
		};

		struct Bundle
		{
			std::set<uint32> ReliableUIDs;
			Timestamp Timestamp = 0;
		};

	public:
		PacketManagerBase(const PacketManagerDesc& desc);

		uint32 EnqueueSegmentReliable(NetworkSegment* pSegment, IPacketListener* pListener = nullptr);
		uint32 EnqueueSegmentUnreliable(NetworkSegment* pSegment);

		void Flush(PacketTransceiverBase* pTransceiver);
		bool QueryBegin(PacketTransceiverBase* pTransceiver, TArray<NetworkSegment*>& segmentsReturned);
		void QueryEnd(TArray<NetworkSegment*>& packetsReceived);

		virtual void Tick(Timestamp delta);

		SegmentPool* GetSegmentPool();
		const NetworkStatistics* GetStatistics() const;
		const IPEndPoint& GetEndPoint() const;
		void SetEndPoint(const IPEndPoint& ipEndPoint);
		virtual void Reset();

	protected:
		virtual bool FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned) = 0;

	private:
		uint32 EnqueueSegment(NetworkSegment* pSegment, uint32 reliableUID);
		void DeleteOldBundles();
		void HandleAcks(const TArray<uint32>& acks);
		void GetReliableUIDsFromAckedPackets(const TArray<uint32>& acks, TArray<uint32>& ackedReliableUIDs);
		void GetReliableSegmentInfosFromUIDs(const TArray<uint32>& ackedReliableUIDs, TArray<SegmentInfo>& ackedReliableSegments);
		void RegisterRTT(Timestamp rtt);

	protected:
		NetworkStatistics m_Statistics;
		SegmentPool m_SegmentPool;
		IPEndPoint m_IPEndPoint;
		std::queue<NetworkSegment*> m_SegmentsToSend[2];
		std::unordered_map<uint32, SegmentInfo> m_SegmentsWaitingForAck;
		std::unordered_map<uint32, Bundle> m_Bundles;
		std::atomic_int m_QueueIndex;
		Timestamp m_Timer;
		SpinLock m_LockSegmentsToSend;
		SpinLock m_LockBundles;
	};
}
