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

	struct NetworkPair
	{
		NetworkPacket* Packet;
		IPacketListener* Listener;
		Timestamp LastSent;
	};

#pragma pack(push, 1)
	struct PacketDispatcherHeader
	{
		uint16 Size = 0;
		uint32 UID = 0;
		uint8 Packets = 0;
	};
#pragma pack(pop)

	class LAMBDA_API PacketDispatcher
	{
	public:
		~PacketDispatcher();

		void EnqueuePacket(NetworkPacket* packet);
		void EnqueuePacket(NetworkPacket* packet, IPacketListener* listener);
		NetworkPacket* GetFreePacket();

	private:
		PacketDispatcher();

		bool Dispatch(ISocketUDP* socket, const IPEndPoint& destination);
		void Clear();

	private:
		std::vector<NetworkPacket*> m_PacketsFree;
		std::queue<NetworkPair> m_PacketsToSend[2];
		std::unordered_map<uint32, NetworkPair> m_PacketsWaitingForAck;

		SpinLock m_LockPacketsFree;
		SpinLock m_LockPacketsToSend;

		std::atomic_int m_QueueIndex;

		char m_pSendBuffer[MAXIMUM_PACKET_SIZE];
	};
}