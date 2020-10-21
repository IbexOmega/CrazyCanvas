#pragma once

#include "ECS/Component.h"
#include "Physics/CollisionGroups.h"

constexpr LambdaEngine::CollisionGroup FLAG_CARRIED_COLLISION_MASK = FCrazyCanvasCollisionGroup::COLLISION_GROUP_BASE;
constexpr LambdaEngine::CollisionGroup FLAG_DROPPED_COLLISION_MASK = FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER;

struct FlagComponent
{
	DECL_COMPONENT(FlagComponent);
};
