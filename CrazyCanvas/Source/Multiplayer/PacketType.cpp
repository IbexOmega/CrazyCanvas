#include "Multiplayer/Packet/PacketType.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"
#include "ECS/Components/Match/FlagComponent.h"

#include "Multiplayer/Packet/PlayerAction.h"
#include "Multiplayer/Packet/PlayerActionResponse.h"
#include "Multiplayer/Packet/CreateLevelObject.h"
#include "Multiplayer/Packet/PacketTeamScored.h"
#include "Multiplayer/Packet/PacketGameOver.h"
#include "Multiplayer/Packet/PacketHostServer.h"

uint16 PacketType::s_PacketTypeCount = 0;
PacketTypeMap PacketType::s_PacketTypeToEvent;

uint16 PacketType::CREATE_LEVEL_OBJECT		= 0;
uint16 PacketType::DELETE_LEVEL_OBJECT		= 0;
uint16 PacketType::PLAYER_ACTION			= 0;
uint16 PacketType::PLAYER_ACTION_RESPONSE	= 0;
uint16 PacketType::FLAG_EDITED				= 0;
uint16 PacketType::TEAM_SCORED				= 0;
uint16 PacketType::GAME_OVER				= 0;
uint16 PacketType::HOST_SERVER				= 0;

void PacketType::Init()
{
	CREATE_LEVEL_OBJECT		= RegisterPacketType<CreateLevelObject>();
	DELETE_LEVEL_OBJECT		= RegisterPacketTypeRaw();
	PLAYER_ACTION			= RegisterPacketTypeWithComponent<PlayerAction>();
	PLAYER_ACTION_RESPONSE	= RegisterPacketTypeWithComponent<PlayerActionResponse>();
	FLAG_EDITED				= RegisterPacketTypeWithComponent<FlagEditedPacket>();
	TEAM_SCORED				= RegisterPacketType<PacketTeamScored>();
	GAME_OVER				= RegisterPacketType<PacketGameOver>();
	HOST_SERVER				= RegisterPacketType<PacketHostServer>();
}

uint16 PacketType::RegisterPacketTypeRaw()
{
	return ++s_PacketTypeCount;
}

IPacketReceivedEvent* PacketType::GetPacketReceivedEventPointer(uint16 packetType)
{
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