#include "Multiplayer/Packet/PacketType.h"

#include "Multiplayer/Packet/PacketCreateLevelObject.h"
#include "Multiplayer/Packet/PacketDeleteLevelObject.h"
#include "Multiplayer/Packet/PacketPlayerAction.h"
#include "Multiplayer/Packet/PacketPlayerActionResponse.h"
#include "Multiplayer/Packet/PacketConfigureServer.h"
#include "Multiplayer/Packet/PacketTeamScored.h"
#include "Multiplayer/Packet/PacketGameOver.h"
#include "Multiplayer/Packet/PacketWeaponFired.h"
#include "Multiplayer/Packet/PacketHealthChanged.h"
#include "Multiplayer/Packet/PacketFlagEdited.h"
#include "Multiplayer/Packet/PacketProjectileHit.h"

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
uint16 PacketType::GAME_OVER				= 0;
uint16 PacketType::CONFIGURE_SERVER			= 0;
uint16 PacketType::PROJECTILE_HIT			= 0;

void PacketType::Init()
{
	CREATE_LEVEL_OBJECT		= RegisterPacketType<PacketCreateLevelObject>();
	DELETE_LEVEL_OBJECT		= RegisterPacketType<PacketDeleteLevelObject>();
	PLAYER_ACTION			= RegisterPacketTypeWithComponent<PacketPlayerAction>();
	PLAYER_ACTION_RESPONSE	= RegisterPacketTypeWithComponent<PacketPlayerActionResponse>();
	WEAPON_FIRE				= RegisterPacketTypeWithComponent<PacketWeaponFired>();
	HEALTH_CHANGED			= RegisterPacketTypeWithComponent<PacketHealthChanged>();
	FLAG_EDITED				= RegisterPacketTypeWithComponent<PacketFlagEdited>();
	TEAM_SCORED				= RegisterPacketType<PacketTeamScored>();
	GAME_OVER				= RegisterPacketType<PacketGameOver>();
	CONFIGURE_SERVER		= RegisterPacketType<PacketConfigureServer>();
	PROJECTILE_HIT			= RegisterPacketType<PacketProjectileHit>();
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