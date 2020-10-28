#include "Multiplayer/Packet/PacketType.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"
#include "ECS/Components/Match/FlagComponent.h"

#include "Multiplayer/Packet/PlayerAction.h"
#include "Multiplayer/Packet/PlayerActionResponse.h"

#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/HealthComponent.h"

uint16 PacketType::s_PacketTypeCount = 0;
PacketTypeMap PacketType::s_PacketTypeToEvent;

uint16 PacketType::CREATE_LEVEL_OBJECT		= 0;
uint16 PacketType::DELETE_LEVEL_OBJECT		= 0;
uint16 PacketType::PLAYER_ACTION			= 0;
uint16 PacketType::PLAYER_ACTION_RESPONSE	= 0;
uint16 PacketType::WEAPON_FIRE				= 0;
uint16 PacketType::HEALTH_CHANGED			= 0;
uint16 PacketType::FLAG_EDITED				= 0;
uint16 PacketType::TEAM_SCORED				= 0;

void PacketType::Init()
{
	CREATE_LEVEL_OBJECT		= RegisterPacketTypeRaw();
	DELETE_LEVEL_OBJECT		= RegisterPacketTypeRaw();
	PLAYER_ACTION			= RegisterPacketTypeWithComponent<PlayerAction>();
	PLAYER_ACTION_RESPONSE	= RegisterPacketTypeWithComponent<PlayerActionResponse>();
	WEAPON_FIRE				= RegisterPacketTypeWithComponent<WeaponFiredPacket>();
	HEALTH_CHANGED			= RegisterPacketTypeWithComponent<HealthChangedPacket>();
	FLAG_EDITED				= RegisterPacketTypeWithComponent<FlagEditedPacket>();
	TEAM_SCORED				= RegisterPacketTypeRaw();
}

uint16 PacketType::RegisterPacketTypeRaw()
{
	return ++s_PacketTypeCount;
}

IPacketReceivedEvent* PacketType::GetPacketReceivedEventPointer(uint16 packetType)
{
	VALIDATE_MSG(packetType != 0 && packetType <= s_PacketTypeCount, "Packet type not registered, have you forgotten to register your package?");

	auto pair = s_PacketTypeToEvent.find(packetType);
	return pair == s_PacketTypeToEvent.end() ? nullptr : pair->second;
}

const PacketTypeMap& PacketType::GetPacketTypeMap()
{
	return s_PacketTypeToEvent;
}

void PacketType::Release()
{
	for (auto& pair : s_PacketTypeToEvent)
	{
		SAFEDELETE(pair.second);
	}
	s_PacketTypeToEvent.clear();
}