#include "Multiplayer/PacketType.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

uint16 PacketType::s_PacketTypeCount = 0;
PacketTypeMap PacketType::s_PacketTypeToComponentType;

uint16 PacketType::CREATE_ENTITY			= 0;
uint16 PacketType::PLAYER_ACTION			= 0;
uint16 PacketType::PLAYER_ACTION_RESPONSE	= 0;

void PacketType::Init()
{
	CREATE_ENTITY			= RegisterPacketType();
	PLAYER_ACTION			= RegisterPacketTypeWithComponent<PlayerAction>();
	PLAYER_ACTION_RESPONSE	= RegisterPacketTypeWithComponent<PlayerActionResponse>();
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
