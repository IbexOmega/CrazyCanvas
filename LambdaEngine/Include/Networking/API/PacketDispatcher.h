#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"
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

	struct NetworkPair
	{
		NetworkPacket* Packet = nullptr;
		IPacketListener* Listener = nullptr;
		Timestamp LastSent;
	};

	struct BundledPacket
	{
		NetworkPair NetworkPairs[32];
		uint8 Count = 0;
	};

#pragma pack(push, 1)
	struct PacketDispatcherHeader
	{
		uint16 Size		= 0;
		uint32 Sequence = 0;
		uint32 Ack		= 0;
		uint32 AckBits	= 0;
		uint8 Packets	= 0;
	};
#pragma pack(pop)

	class LAMBDA_API PacketDispatcher
	{
	public:
		PacketDispatcher(uint16 packets, IDispatcherHandler* pHandler);
		~PacketDispatcher();

		void EnqueuePacket(NetworkPacket* packet);
		void EnqueuePacket(NetworkPacket* packet, IPacketListener* listener);
		NetworkPacket* GetFreePacket();

		bool Dispatch(ISocketUDP* socket, const IPEndPoint& destination);
		bool Receive(ISocketUDP* socket);

	private:
		bool Send(ISocketUDP* socket, const IPEndPoint& destination, PacketDispatcherHeader& header, BundledPacket& bundle);

		void ProcessSequence(uint32 sequence);
		void ProcessAcks(uint32 ack, uint32 ackBits);

		bool GetAndRemoveBundle(uint32 sequence, BundledPacket& bundle);
		uint32 GetNextPacketSequenceNr();
		uint32 GetNextMessageUID();
		void Clear();

	private:
		NetworkPacket* m_pPackets;
		std::vector<NetworkPacket*> m_PacketsFree;
		std::queue<NetworkPair> m_PacketsToSend[2];
		std::unordered_map<uint32, BundledPacket> m_PacketsWaitingForAck;

		SpinLock m_LockPacketsFree;
		SpinLock m_LockPacketsToSend;
		SpinLock m_LockPacketsWaitingForAck;

		std::atomic_int m_QueueIndex;

		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
		char m_pReceiveBuffer[UINT16_MAX_];

		uint32 m_PacketsCounter;
		uint32 m_MessageCounter;
		std::atomic_uint32_t m_LastReceivedSequenceNr;

		bool m_ReceivedHistory[32];

		IDispatcherHandler* m_pHandler;
	};
}