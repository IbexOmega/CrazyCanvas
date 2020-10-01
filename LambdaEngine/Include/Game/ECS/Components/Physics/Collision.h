#pragma once

#include "ECS/Component.h"

namespace physx
{
	class PxActor;
}

namespace LambdaEngine
{
	struct CollisionComponent
	{
		DECL_COMPONENT(CollisionComponent);
		physx::PxActor* pActor;
	};
}
