#pragma once
#include "ECS/Component.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Physics/CollisionGroups.h"

#include "Multiplayer/Packet/Packet.h"

#include "Time/API/Timestamp.h"

enum EFlagColliderType : uint8
{
	FLAG_COLLIDER_TYPE_PLAYER			= 0,
	FLAG_COLLIDER_TYPE_DELIVERY_POINT	= 1,
};

struct FlagSpawnComponent
{
	DECL_COMPONENT(FlagSpawnComponent);
};

struct FlagDeliveryPointComponent
{
	DECL_COMPONENT(FlagDeliveryPointComponent);
};

struct FlagComponent
{
	DECL_COMPONENT(FlagComponent);
	LambdaEngine::Timestamp DroppedTimestamp;
	LambdaEngine::Timestamp PickupCooldown;
	LambdaEngine::Timestamp RespawnCooldown;
	bool HasBeenPickedUp = false;
};

enum EFlagPacketType : uint8
{
	FLAG_PACKET_TYPE_PICKED_UP	= 0,
	FLAG_PACKET_TYPE_DROPPED	= 1,
};