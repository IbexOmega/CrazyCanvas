#include "Game/Multiplayer/MultiplayerUtilBase.h"

#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	void MultiplayerUtilBase::SubscribeToPacketType(uint16 packetType, const PacketFunction& func)
	{
		m_PacketSubscribers[packetType].PushBack(func);
	}

	void MultiplayerUtilBase::FirePacketEvent(IClient* pClient, NetworkSegment* pPacket)
	{
		auto iterator = m_PacketSubscribers.find(pPacket->GetType());
		if (iterator != m_PacketSubscribers.end())
		{
			const TArray<PacketFunction>& functions = iterator->second;
			for (const auto& func : functions)
			{
				func(pClient, pPacket);
			}
		}
		else
		{
			LOG_WARNING("No packet subscription of type: %hu", pPacket->GetType());
		}
	}
}
