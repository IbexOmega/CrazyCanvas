#pragma once
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include "Math/Math.h"

#include "Resources/Mesh.h"
#include "Resources/AnimationBlendState.h"

namespace LambdaEngine
{
	/*
	* AnimationComponent
	*/

	struct AnimationComponent
	{
		DECL_COMPONENT(AnimationComponent);

		bool				IsPaused	= false;
		float64				StartTime	= 0.0;
		AnimationState		State;
		SkeletonPose		Pose;
	};
}