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

	struct CharacterColliderComponent
	{
		DECL_COMPONENT(CharacterColliderComponent);
		physx::PxController* pController;
		physx::PxControllerFilters Filters;
	};

	/*
	* A duplicate of the Local Player used for prediction. Should not be used on other Entities than the Local Player 
	*/
	struct CharacterLocalColliderComponent : public CharacterColliderComponent
	{
		DECL_COMPONENT(CharacterLocalColliderComponent);
	};
}
