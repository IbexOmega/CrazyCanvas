#pragma once
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include "Math/Math.h"

#include "Resources/Mesh.h"

namespace LambdaEngine
{
	struct AnimationComponent
	{
		DECL_COMPONENT(AnimationComponent);

		GUID_Lambda			AnimationGUID	= GUID_NONE;
		float64				DurationInTicks = 0.0f;
		TArray<glm::mat4>	BoneMatrices;
	};
}