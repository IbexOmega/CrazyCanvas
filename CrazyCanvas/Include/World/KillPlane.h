#pragma once

#include "ECS/ECSCore.h"
#include "ECS/Entity.h"

// KillPlaneCallback is called when an entity falls behind a kill plane. Entity0 is the kill plane entity.
inline void KillPlaneCallback(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	UNREFERENCED_VARIABLE(entity0);
	LOG_INFO("Entity %d was deleted by a kill plane", entity1);
	LambdaEngine::ECSCore::GetInstance()->RemoveEntity(entity1);
}
