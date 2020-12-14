#pragma once

#include "Defines.h"

#include <stdint.h>

namespace LambdaEngine
{
	struct MemoryStats
	{
		size_t PeakWorkingSetSize;
		size_t CurrentWorkingSetSize;
	};

	class LAMBDA_API RuntimeStats
	{
	public:
		DECL_STATIC_CLASS(RuntimeStats);

		static void SetFrameTime(float frameTime);

		static float GetAverageFrametime() { return m_AverageFrametime; }
		static size_t GetAverageMemoryUsage() { return m_AverageRAMUsage; }
		static size_t GetPeakMemoryUsage();

	private:
		static MemoryStats GetCurrentMemoryStats();

	private:
		static uint64_t m_FrameCount;
		static float m_AverageFrametime;

		static size_t m_AverageRAMUsage;
	};
}
