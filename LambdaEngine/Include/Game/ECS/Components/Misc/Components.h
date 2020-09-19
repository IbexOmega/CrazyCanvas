#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct TrackComponent
	{
		DECL_COMPONENT(TrackComponent);
		std::vector<glm::vec3> Track;
		size_t CurrentTrackIndex = 0;
		float32 CurrentTrackT = 0.0f;
		bool HasReachedEnd = false;
	};
}