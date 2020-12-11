#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	FORCEINLINE uint64 AlignUp(uint64 value, uint64 alignment)
	{
		const uint64 mask = alignment - 1;
		return ((value + mask) & (~mask));
	}

	FORCEINLINE uint64 AlignDown(uint64 value, uint64 alignment)
	{
		const uint64 mask = alignment - 1;
		return ((value) & (~mask));
	}

	FORCEINLINE void GenerateFibonacciSphere(glm::vec3* pSpherePoints, uint32 numPoints)
	{
		static float32 goldenAngle = (2.0f - glm::golden_ratio<float32>()) * (2.0f * glm::pi<float32>());
		
		const float32 numPointsPlus1Inverse = 1.0f / float32(numPoints + 1);

		for (uint32 i = 0; i < numPoints; i++)
		{
			const float32 iPlus1		= float32(i + 1);
			const float32 latitude		= glm::asin(-1.0f + 2.0f * iPlus1 * numPointsPlus1Inverse);
			const float32 longitude		= goldenAngle * iPlus1;

			glm::vec3& position = pSpherePoints[i];
			position.x = glm::cos(longitude) * glm::cos(latitude);
			position.y = glm::sin(longitude) * glm::cos(latitude);
			position.z = glm::sin(latitude);
		}
	}

	FORCEINLINE glm::vec3 GenerateFibonacciSpherePoint(uint32 index, uint32 totalNumPoints)
	{
		static float32 goldenAngle = (2.0f - glm::golden_ratio<float32>()) * (2.0f * glm::pi<float32>());

		VALIDATE(index < totalNumPoints);

		const float32 numPointsPlus1Inverse = 1.0f / float32(totalNumPoints + 1);
		const float32 iPlus1 = float32(index + 1);
		const float32 latitude = glm::asin(-1.0f + 2.0f * iPlus1 * numPointsPlus1Inverse);
		const float32 longitude = goldenAngle * iPlus1;

		return glm::vec3(
			glm::cos(longitude) * glm::cos(latitude),
			glm::sin(longitude) * glm::cos(latitude),
			glm::sin(latitude));
	}
}
