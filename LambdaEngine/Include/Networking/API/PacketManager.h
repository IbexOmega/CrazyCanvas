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

	class LAMBDA_API PacketManager
	{
	public:
		struct MessageInfo
		{
			NetworkPacket* Packet		= nullptr;
			IPacketListener* Listener	= nullptr;
			Timestamp LastSent			= 0;
			uint8 Tries					= 0;
		};

		struct Bundle
		{
			std::set<uint32> ReliableUIDs;
			Timestamp Timestamp = 0;
		};

	public:
		PacketManager();
		~PacketManager();

		uint32 EnqueuePacketReliable(NetworkPacket* pPacket, IPacketListener* pListener = nullptr);
		uint32 EnqueuePacketUnreliable(NetworkPacket* pPacket);

		void Flush(PacketTransceiver* pTransceiver);
		void QueryBegin(PacketTransceiver* pTransceiver, std::vector<NetworkPacket*>& packetsReturned);
		void QueryEnd(std::vector<NetworkPacket*>& packetsReceived);

		void Tick();

		PacketPool* GetPacketPool();
		const NetworkStatistics* GetStatistics() const;
		const IPEndPoint& GetEndPoint() const;

		void SetEndPoint(const IPEndPoint& ipEndPoint);

		void Reset();

	private:
		uint32 EnqueuePacket(NetworkPacket* pPacket, uint32 reliableUID);
		void FindPacketsToReturn(const std::vector<NetworkPacket*>& packetsReceived, std::vector<NetworkPacket*>& packetsReturned);
		void UntangleReliablePackets(std::vector<NetworkPacket*>& packetsReturned);
		void HandleAcks(const std::vector<uint32>& acks);
		void GetReliableUIDsFromAcks(const std::vector<uint32>& acks, std::vector<uint32>& ackedReliableUIDs);
		void GetReliableMessageInfosFromUIDs(const std::vector<uint32>& ackedReliableUIDs, std::vector<MessageInfo>& ackedReliableMessages);

	private:
		NetworkStatistics m_Statistics;
		PacketPool m_PacketPool;
		IPEndPoint m_IPEndPoint;
		std::queue<NetworkPacket*> m_MessagesToSend[2];
		std::unordered_map<uint32, MessageInfo> m_MessagesWaitingForAck;
		std::set<NetworkPacket*> m_ReliableMessagesReceived;
		std::unordered_map<uint32, Bundle> m_Bundles;
		std::atomic_int m_QueueIndex;
		SpinLock m_LockMessagesToSend;
		SpinLock m_LockBundles;
	};
}