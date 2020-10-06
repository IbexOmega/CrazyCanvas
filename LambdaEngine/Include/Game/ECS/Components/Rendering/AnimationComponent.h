#pragma once
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include "Math/Math.h"

#include "Resources/Mesh.h"

#define INFINITE_LOOPS -1

namespace LambdaEngine
{
	/*
	* AnimationComponent
	*/

	struct AnimationComponent
	{
		DECL_COMPONENT(AnimationComponent);

		bool			IsPaused				= false;
		bool			IsLooping				= true;
		int32			NumLoops				= INFINITE_LOOPS;
		GUID_Lambda		AnimationGUID			= GUID_NONE;
		GUID_Lambda		BlendingAnimationGUID	= GUID_NONE;
		float64			StartTime				= 0.0;
		float32			PlaybackSpeed			= 1.0f;
		SkeletonPose	Pose;
	};
}