#pragma once

#include "Types.h"

#include "ECS/Entity.h"

#include "Game/Multiplayer/PacketFunction.h"

namespace LambdaEngine
{
	typedef std::unordered_map<uint16, TArray<PacketFunction>> PacketSubscriberMap;

	class MultiplayerUtilBase
	{
		friend class ClientSystem;
		friend class ServerSystem;

	public:
		DECL_ABSTRACT_CLASS(MultiplayerUtilBase);

		virtual Entity GetEntity(int32 networkUID) const = 0;
		void SubscribeToPacketType(uint16 packetType, const PacketFunction& func);

	private:
		void FirePacketEvent(IClient* pClient, NetworkSegment* pPacket);

	private:
		PacketSubscriberMap m_PacketSubscribers;
	};
}