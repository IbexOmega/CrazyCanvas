#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"

#include "Threading/API/SpinLock.h"

//#define DEBUG_PACKET_POOL

namespace LambdaEngine
{
	class NetworkPacket;

	class LAMBDA_API PacketPool
	{
	public:
		PacketPool(uint16 size);
		~PacketPool();

		NetworkPacket* RequestFreePacket();
		bool RequestFreePackets(uint16 nrOfPackets, TArray<NetworkPacket*>& packetsReturned);

		void FreePacket(NetworkPacket* pPacket);
		void FreePackets(TArray<NetworkPacket*>& packets);
	
		void Reset();

		uint16 GetSize() const;
		uint16 GetFreePackets() const;

	private:
		void Request(NetworkPacket* pPacket);
		void Free(NetworkPacket* pPacket);

	private:
		TArray<NetworkPacket*> m_Packets;
		TArray<NetworkPacket*> m_PacketsFree;
		SpinLock m_Lock;
	};
}