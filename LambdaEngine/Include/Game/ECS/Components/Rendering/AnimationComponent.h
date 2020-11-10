#pragma once
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include "Math/Math.h"

#include "Resources/Mesh.h"

#include "Rendering/Animation/AnimationGraph.h"

#ifndef LAMBDA_DEBUG
	#define USE_ALL_ANIMATIONS
#endif

namespace LambdaEngine
{
	/*
	* AnimationComponent
	*/

	struct AnimationComponent
	{
		DECL_COMPONENT(AnimationComponent);

		bool			IsPaused = false;
		AnimationGraph*	pGraph = nullptr;
		SkeletonPose	Pose;
	};

	/*
	* AttachedAnimationComponent
	*/
	struct AnimationAttachedComponent
	{
		DECL_COMPONENT(AnimationAttachedComponent);

		String		JointName = "";
		glm::mat4	Transform;
	};
}