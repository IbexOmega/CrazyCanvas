#pragma once

#include "LambdaEngine.h"
#include "Containers/TSet.h"
#include "Containers/THashTable.h"
#include "Containers/TQueue.h"

#include "Networking/API/NetworkPacket.h"
#include "Networking/API/IPEndPoint.h"

#include "Threading/API/SpinLock.h"

#include <atomic>

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class IPacketListener;
	class ISocketUDP;
	class IDispatcherHandler;

	class LAMBDA_API PacketManager
	{
		struct MessageInfo
		{
			NetworkPacket*	Packet		= nullptr;
			IPacketListener* Listener	= nullptr;
			Timestamp LastSent			= 0;
			uint8 Tries					= 0;

			uint32 GetUID() const
			{
				return Packet->GetHeader().UID;
			}
		};

		struct Bundle
		{
			std::set<uint32> MessageUIDs;
			Timestamp Timestamp = 0;
		};

#pragma pack(push, 1)
		struct Header
		{
			uint16 Size		= 0;
			uint64 Salt		= 0;
			uint32 Sequence = 0;
			uint32 Ack		= 0;
			uint32 AckBits	= 0;
			uint8  Packets	= 0;
		};
#pragma pack(pop)


	public:
		PacketManager(uint16 packets, uint8 maximumTries);
		~PacketManager();

		void EnqueuePacket(NetworkPacket* packet);
		void EnqueuePacket(NetworkPacket* packet, IPacketListener* listener);
		NetworkPacket* GetFreePacket();

		bool EncodePackets(char* buffer, int32& bytesWritten);
		bool DecodePackets(const char* buffer, int32 bytesReceived, NetworkPacket** packetsRead, int32& nrOfPackets);
		void SwapPacketQueues();
		void Tick();

		void Free(NetworkPacket** packets, int32 nrOfPackets);

		const Timestamp& GetPing() const;

		uint64 GetSalt() const;

		void Reset();

	private:
		void WriteHeaderAndStoreBundle(char* buffer, int32& bytesWritten, Header& header, Bundle& bundle, std::vector<MessageInfo>& reliableMessages);

		void ProcessSequence(uint32 sequence);
		void ProcessAcks(uint32 ack, uint32 ackBits);
		void ProcessAck(uint32 ack, Timestamp& rtt);

		bool GetMessagesAndRemoveBundle(uint32 sequence, std::vector<MessageInfo>& messages, Timestamp& sentTimestamp);
		uint32 GetNextPacketSequenceNr();
		uint32 GetNextMessageUID();

		void DeleteEmptyBundles();
		void FindMessagesToResend(std::vector<MessageInfo>& messages);
		void ReSendMessages(const std::vector<MessageInfo>& messages);

	private:
		uint16 m_NrOfPackets;
		NetworkPacket* m_pPackets;
		std::set<NetworkPacket*> m_PacketsFree;
		std::queue<MessageInfo> m_PacketsToSend[2];
		std::unordered_map<uint32, Bundle> m_PacketsWaitingForAck;
		std::unordered_map<uint32, MessageInfo> m_MessagesWaitingForAck;

		SpinLock m_LockPacketsFree;
		SpinLock m_LockPacketsToSend;
		SpinLock m_LockPacketsWaitingForAck;

		std::atomic_int m_QueueIndex;

		uint32 m_PacketsCounter;
		uint32 m_MessageCounter;
		std::atomic_uint32_t m_LastReceivedSequenceNr;
		std::atomic_uint32_t m_ReceivedSequenceBits;

		Timestamp m_Ping;

		std::atomic_uint64_t m_Salt;
		std::atomic_uint64_t m_SaltRemote;

		uint8 m_MaximumTries;

	public:
		static uint64 DoChallenge(uint64 clientSalt, uint64 serverSalt);
	};
}