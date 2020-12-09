#pragma once

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4201) //Disable nameless struct/union warning
	#pragma warning(disable : 4251) //Disable the DLL- linkage warning for now
#endif

#define GLM_FORCE_INLINE
#define GLM_FORCE_SSE2
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "MathUtilities.h"

namespace LambdaEngine
{
	class Math
	{
	public:
		FORCEINLINE static float32 RadicalInverse(uint32 bits)
		{
			bits = (bits << 16u) | (bits >> 16u);
			bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
			bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
			bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
			bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
			return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
		}

		FORCEINLINE static glm::vec2 Hammersley2D(uint64 sampleIdx, uint64 numSamples)
		{
			return glm::vec2(float32(sampleIdx) / float32(numSamples), RadicalInverse(uint32(sampleIdx)));
		}
	};
}