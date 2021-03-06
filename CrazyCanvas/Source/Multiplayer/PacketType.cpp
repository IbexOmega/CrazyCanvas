#include "Multiplayer/Packet/PacketType.h"

#include "Networking/API/NetworkDebugger.h"

#include "Multiplayer/Packet/PacketCreateLevelObject.h"
#include "Multiplayer/Packet/PacketDeleteLevelObject.h"
#include "Multiplayer/Packet/PacketPlayerAction.h"
#include "Multiplayer/Packet/PacketPlayerActionResponse.h"
#include "Multiplayer/Packet/PacketGameSettings.h"
#include "Multiplayer/Packet/PacketTeamScored.h"
#include "Multiplayer/Packet/PacketMatchReady.h"
#include "Multiplayer/Packet/PacketMatchStart.h"
#include "Multiplayer/Packet/PacketMatchBegin.h"
#include "Multiplayer/Packet/PacketGameOver.h"
#include "Multiplayer/Packet/PacketWeaponFired.h"
#include "Multiplayer/Packet/PacketHealthChanged.h"
#include "Multiplayer/Packet/PacketFlagEdited.h"
#include "Multiplayer/Packet/PacketJoin.h"
#include "Multiplayer/Packet/PacketLeave.h"
#include "Multiplayer/Packet/PacketPlayerAliveChanged.h"
#include "Multiplayer/Packet/PacketPlayerHost.h"
#include "Multiplayer/Packet/PacketPlayerPing.h"
#include "Multiplayer/Packet/PacketPlayerReady.h"
#include "Multiplayer/Packet/PacketPlayerScore.h"
#include "Multiplayer/Packet/PacketPlayerState.h"
#include "Multiplayer/Packet/PacketPositionPing.h"
#include "Multiplayer/Packet/PacketProjectileHit.h"
#include "Multiplayer/Packet/PacketResetPlayerTexture.h"
#include "Multiplayer/Packet/PacketSessionSettingChanged.h"
#include "Multiplayer/Packet/PacketGrenadeThrown.h"

uint16 PacketType::s_PacketTypeCount = 0;
PacketTypeMap PacketType::s_PacketTypeToEvent;

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
	MATCH_READY				= RegisterPacketType<PacketMatchReady>();
	MATCH_START				= RegisterPacketType<PacketMatchStart>();
	MATCH_BEGIN				= RegisterPacketType<PacketMatchBegin>();
	GAME_OVER				= RegisterPacketType<PacketGameOver>();
	GAME_SETTINGS			= RegisterPacketType<PacketGameSettings>();
	JOIN					= RegisterPacketType<PacketJoin>();
	LEAVE					= RegisterPacketType<PacketLeave>();
	CHAT_MESSAGE			= RegisterPacketTypeRaw("CHAT_MESSAGE");
	PLAYER_ALIVE_CHANGED	= RegisterPacketType<PacketPlayerAliveChanged>();
	PLAYER_HOST				= RegisterPacketType<PacketPlayerHost>();
	PLAYER_PING				= RegisterPacketType<PacketPlayerPing>();
	PLAYER_POSITION_PING	= RegisterPacketType<PacketPositionPing>();
	PLAYER_READY			= RegisterPacketType<PacketPlayerReady>();
	PLAYER_SCORE			= RegisterPacketType<PacketPlayerScore>();
	PLAYER_STATE			= RegisterPacketType<PacketPlayerState>();
	PROJECTILE_HIT			= RegisterPacketType<PacketProjectileHit>();
	RESET_PLAYER_TEXTURE	= RegisterPacketTypeWithComponent<PacketResetPlayerTexture>();
	SESSION_SETTING_CHANGED	= RegisterPacketType<PacketSessionSettingChanged>();
	GRENADE_THROWN			= RegisterPacketType<PacketGrenadeThrown>();
}

uint16 PacketType::RegisterPacketTypeRaw(const char* pName)
{
	LambdaEngine::NetworkDebugger::RegisterPacketName(++s_PacketTypeCount, pName);
	return s_PacketTypeCount;
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