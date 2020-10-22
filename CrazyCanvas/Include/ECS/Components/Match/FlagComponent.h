#pragma once

#include "ECS/Component.h"
#include "Physics/CollisionGroups.h"

#include "Multiplayer/Packet.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"

constexpr LambdaEngine::CollisionGroup FLAG_CARRIED_COLLISION_MASK = FCrazyCanvasCollisionGroup::COLLISION_GROUP_BASE;
constexpr LambdaEngine::CollisionGroup FLAG_DROPPED_COLLISION_MASK = FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER;

struct FlagComponent
{
	DECL_COMPONENT(FlagComponent);
};

enum EFlagPacketType : uint8
{
	FLAG_PACKET_TYPE_PICKED_UP	= 0,
	FLAG_PACKET_TYPE_DROPPED	= 1,
};

#pragma pack(push, 1)
struct FlagEditedPacket : Packet
{
	EFlagPacketType	FlagPacketType;
	int32			PickedUpNetworkUID = -1;
	glm::vec3		DroppedPosition;
};
#pragma pack(pop)