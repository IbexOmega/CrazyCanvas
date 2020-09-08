#pragma once

#include "LambdaEngine.h"
#include "Containers/TQueue.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include "Containers/THashTable.h"

#include "Networking/API/NetworkStatistics.h"
#include "Networking/API/PacketPool.h"
#include "Networking/API/IPEndPoint.h"

#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	class NetworkPacket;
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
		struct MessageInfo
		{
			NetworkPacket* Packet		= nullptr;
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

		uint32 EnqueuePacketReliable(NetworkPacket* pPacket, IPacketListener* pListener = nullptr);
		uint32 EnqueuePacketUnreliable(NetworkPacket* pPacket);

		void Flush(PacketTransceiver* pTransceiver);
		void QueryBegin(PacketTransceiver* pTransceiver, TArray<NetworkPacket*>& packetsReturned);
		void QueryEnd(TArray<NetworkPacket*>& packetsReceived);

		void Tick(Timestamp delta);

		PacketPool* GetPacketPool();
		const NetworkStatistics* GetStatistics() const;
		const IPEndPoint& GetEndPoint() const;

		void SetEndPoint(const IPEndPoint& ipEndPoint);

		void Reset();

	private:
		uint32 EnqueuePacket(NetworkPacket* pPacket, uint32 reliableUID);
		void FindPacketsToReturn(const TArray<NetworkPacket*>& packetsReceived, TArray<NetworkPacket*>& packetsReturned);
		void UntangleReliablePackets(TArray<NetworkPacket*>& packetsReturned);
		void HandleAcks(const TArray<uint32>& acks);
		void GetReliableUIDsFromAcks(const TArray<uint32>& acks, TArray<uint32>& ackedReliableUIDs);
		void GetReliableMessageInfosFromUIDs(const TArray<uint32>& ackedReliableUIDs, TArray<MessageInfo>& ackedReliableMessages);
		void RegisterRTT(Timestamp rtt);
		void DeleteOldBundles();
		void ResendOrDeleteMessages();

	private:
		NetworkStatistics m_Statistics;
		PacketPool m_PacketPool;
		IPEndPoint m_IPEndPoint;
		std::queue<NetworkPacket*> m_MessagesToSend[2];
		std::unordered_map<uint32, MessageInfo> m_MessagesWaitingForAck;
		std::set<NetworkPacket*> m_ReliableMessagesReceived;
		std::unordered_map<uint32, Bundle> m_Bundles;
		std::atomic_int m_QueueIndex;
		Timestamp m_Timer;
		float32 m_ResendRTTMultiplier;
		int32 m_MaxRetries;
		SpinLock m_LockMessagesToSend;
		SpinLock m_LockBundles;
	};
}
