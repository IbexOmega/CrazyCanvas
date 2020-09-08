#pragma once

#include "Defines.h"

#include <stdint.h>

namespace LambdaEngine
{
	class LAMBDA_API RuntimeStats
	{
	public:
		DECL_STATIC_CLASS(RuntimeStats);

		static void SetFrameTime(float frameTime);

		static float GetAverageFrametime() { return m_AverageFrametime; }
		static size_t GetPeakMemoryUsage();

	private:
		static uint64_t m_FrameCount;
		static float m_AverageFrametime;
	};
}