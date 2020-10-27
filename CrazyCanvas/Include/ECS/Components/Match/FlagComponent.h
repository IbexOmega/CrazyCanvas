#pragma once

#include "ECS/Component.h"
#include "Physics/CollisionGroups.h"

#include "Multiplayer/Packet/Packet.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Time/API/Timestamp.h"

enum EFlagColliderType : uint8
{
	FLAG_COLLIDER_TYPE_PLAYER			= 0,
	FLAG_COLLIDER_TYPE_DELIVERY_POINT	= 1,
};

struct FlagSpawnComponent
{
	DECL_COMPONENT(FlagSpawnComponent);
	float Radius = 1.0f;
};

struct FlagDeliveryPointComponent
{
	DECL_COMPONENT(FlagDeliveryPointComponent);
};

struct FlagComponent
{
	DECL_COMPONENT(FlagComponent);
	LambdaEngine::Timestamp PickupAvailableTimestamp;
	LambdaEngine::Timestamp PickupCooldown;
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