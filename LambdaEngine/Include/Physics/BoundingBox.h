#pragma once

#include "Math/Math.h"

namespace LambdaEngine
{
	struct BoundingBox
	{
		glm::vec3 Centroid;
		glm::vec3 Dimensions;

		inline void Scale(const glm::mat4& transform)
		{
			Centroid	= transform * glm::vec4(Centroid, 0.0f);
			Dimensions	= transform * glm::vec4(Dimensions, 0.0f);
		}
	};
}
