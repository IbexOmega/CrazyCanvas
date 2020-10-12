#pragma once

#include "ECS/Component.h"

#include <characterkinematic/PxController.h>

namespace LambdaEngine
{
	struct StaticCollisionComponent
	{
		DECL_COMPONENT(StaticCollisionComponent);
		physx::PxActor* pActor;
	};

	struct DynamicCollisionComponent
	{
		DECL_COMPONENT(DynamicCollisionComponent);
		physx::PxRigidDynamic* pActor;
	};

	struct CharacterColliderComponent
	{
		DECL_COMPONENT(CharacterColliderComponent);
		physx::PxController* pController;
		physx::PxControllerFilters Filters;
	};
}
