#pragma once

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

enum FCrazyCanvasCollisionGroup : LambdaEngine::CollisionGroup
{
	COLLISION_GROUP_PLAYER	= (1 << (LambdaEngine::COLLISION_GROUP_LAST + 1)),
	COLLISION_GROUP_FLAG	= (1 << (LambdaEngine::COLLISION_GROUP_LAST + 2)),
	COLLISION_GROUP_BASE	= (1 << (LambdaEngine::COLLISION_GROUP_LAST + 3)),
	COLLISION_GROUP_OTHERS	= (1 << (LambdaEngine::COLLISION_GROUP_LAST + 4)), // Other players, not including oneself
};