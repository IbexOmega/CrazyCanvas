#pragma once

#include "Multiplayer/Packet/Packet.h"
#include "Math/Math.h"
#include "World/LevelObjectCreator.h"

namespace Obj
{
#pragma pack(push, 1)
	struct Player
	{
		bool IsMySelf;
		uint8 TeamIndex;
		int32 WeaponNetworkUID;
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct Flag
	{
		int32 ParentNetworkUID;
	};
#pragma pack(pop)
}

#pragma pack(push, 1)
struct CreateLevelObject
{
	DECL_PACKET(CreateLevelObject);

	ELevelObjectType LevelObjectType;
	int32 NetworkUID;
	glm::vec3 Position;
	glm::vec3 Forward;

#pragma pack(push, 1)
	union
	{
		Obj::Player Player;
		Obj::Flag Flag;
	};
#pragma pack(pop)
};
#pragma pack(pop)