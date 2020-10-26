#include "Multiplayer/Packet/PacketType.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"
#include "ECS/Components/Match/FlagComponent.h"

#include "Multiplayer/Packet/PlayerAction.h"
#include "Multiplayer/Packet/PlayerActionResponse.h"

uint16 PacketType::s_PacketTypeCount = 0;
PacketTypeMap PacketType::s_PacketTypeToComponentType;

uint16 PacketType::CREATE_LEVEL_OBJECT		= 0;
uint16 PacketType::PLAYER_ACTION			= 0;
uint16 PacketType::PLAYER_ACTION_RESPONSE	= 0;
uint16 PacketType::FLAG_EDITED				= 0;

void PacketType::Init()
{
	CREATE_LEVEL_OBJECT		= RegisterPacketType();
	PLAYER_ACTION			= RegisterPacketTypeWithComponent<PlayerAction>();
	PLAYER_ACTION_RESPONSE	= RegisterPacketTypeWithComponent<PlayerActionResponse>();
	FLAG_EDITED				= RegisterPacketTypeWithComponent<FlagEditedPacket>();
}

uint16 PacketType::RegisterPacketType()
{
	return ++s_PacketTypeCount;
}

const PacketTypeMap& PacketType::GetPacketTypeMap()
{
	return s_PacketTypeToComponentType;
}

const LambdaEngine::ComponentType* PacketType::GetComponentType(uint16 packetType)
{
	auto pair = s_PacketTypeToComponentType.find(packetType);
	return pair == s_PacketTypeToComponentType.end() ? nullptr : pair->second;
}
