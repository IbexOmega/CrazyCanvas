#pragma once
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include "Math/Math.h"

#include "Resources/Mesh.h"

namespace LambdaEngine
{
	/*
	* AnimationComponent
	*/

	struct AnimationComponent
	{
		DECL_COMPONENT(AnimationComponent);

		GUID_Lambda		AnimationGUID	= GUID_NONE;
		float64			DurationInTicks	= 0.0f;
		float32			PlaybackSpeed	= 1.0f;
		bool			IsPaused		= false;
		SkeletonPose	Pose;
	};
}