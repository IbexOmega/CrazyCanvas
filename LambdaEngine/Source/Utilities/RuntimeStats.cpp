#include "Utilities/RuntimeStats.h"

#include "Log/Log.h"

#include <algorithm>

#ifdef LAMBDA_PLATFORM_WINDOWS
	#define NOMINMAX
	#include <Windows.h>
	#include <Psapi.h>
#endif

namespace LambdaEngine
{
	uint64_t RuntimeStats::m_FrameCount     = 1;
	float RuntimeStats::m_AverageFrametime  = 0.0f;
	size_t RuntimeStats::m_AverageRAMUsage  = 0;

	void RuntimeStats::SetFrameTime(float frameTime)
	{
		m_AverageFrametime = m_AverageFrametime + (frameTime - m_AverageFrametime) / m_FrameCount;

		const int64 averageRAMUsage = (int64)m_AverageRAMUsage;
		m_AverageRAMUsage = m_AverageRAMUsage + int64(((int64)GetCurrentMemoryStats().CurrentWorkingSetSize - averageRAMUsage) / (int64)m_FrameCount);

		m_FrameCount += 1;
	}

	size_t RuntimeStats::GetPeakMemoryUsage()
	{
		return GetCurrentMemoryStats().PeakWorkingSetSize;
	}

	MemoryStats RuntimeStats::GetCurrentMemoryStats()
	{
		MemoryStats memStats = {};

		#ifdef LAMBDA_PLATFORM_WINDOWS
			PROCESS_MEMORY_COUNTERS memInfo;
			const BOOL result = GetProcessMemoryInfo(GetCurrentProcess(),
				&memInfo,
				sizeof(memInfo));

			if (!result)
			{
				LOG_WARNING("Failed to retrieve process memory info");
			}

			memStats =
			{
				.PeakWorkingSetSize = (size_t)memInfo.PeakWorkingSetSize,
				.CurrentWorkingSetSize = (size_t)memInfo.WorkingSetSize
			};

		#else
			LOG_WARNING("Retrieving memory usage not yet supported on platforms other than Windows");
		#endif

		return memStats;
	}
}
