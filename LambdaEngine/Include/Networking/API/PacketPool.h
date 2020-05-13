#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"

#include "Threading/API/SpinLock.h"

namespace LambdaEngine
{
	class NetworkPacket;

	class LAMBDA_API PacketPool
	{
	public:
		PacketPool(uint16 size);
		~PacketPool();

		NetworkPacket* RequestFreePacket();
		bool RequestFreePackets(uint16 nrOfPackets, std::vector<NetworkPacket*>& packetsReturned);

		void FreePacket(NetworkPacket* pPacket);
		void FreePackets(std::vector<NetworkPacket*>& packets);
	
		void Reset();

		uint16 GetSize() const;
		uint16 GetFreePackets() const;

	private:
		std::vector<NetworkPacket*> m_Packets;
		std::vector<NetworkPacket*> m_PacketsFree;
		SpinLock m_Lock;
	};
}