#pragma once
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include "Math/Math.h"

#include "Resources/Mesh.h"
#include "Resources/AnimationGraph.h"

namespace LambdaEngine
{
	/*
	* AnimationComponent
	*/

	struct AnimationComponent
	{
		DECL_COMPONENT(AnimationComponent);

		bool			IsPaused = false;
		AnimationGraph	Graph;
		SkeletonPose	Pose;
	};
}