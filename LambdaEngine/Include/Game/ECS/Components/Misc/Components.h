#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct TrackComponent
	{
		DECL_COMPONENT(TrackComponent);
		TArray<glm::vec3> Track;
		uint32 CurrentTrackIndex = 0;
		float32 CurrentTrackT = 0.0f;
		bool HasReachedEnd = false;
	};
}