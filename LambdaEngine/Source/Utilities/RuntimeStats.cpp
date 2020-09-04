#include "Utilities/RuntimeStats.h"

#include "Log/Log.h"

#include <algorithm>

#ifdef LAMBDA_PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
    #include <Psapi.h>
#endif

uint64_t RuntimeStats::m_FrameCount     = 1u;
float RuntimeStats::m_AverageFrametime  = 0.0f;

void RuntimeStats::SetFrameTime(float frameTime)
{
    m_AverageFrametime = m_AverageFrametime + (frameTime - m_AverageFrametime) / std::max(1ull, m_FrameCount);
    m_FrameCount += 1;
}

size_t RuntimeStats::GetPeakMemoryUsage()
{
    #ifdef LAMBDA_PLATFORM_WINDOWS
        PROCESS_MEMORY_COUNTERS memInfo;
        BOOL result = GetProcessMemoryInfo(GetCurrentProcess(),
            &memInfo,
            sizeof(memInfo));

        if (!result)
        {
            LOG_WARNING("Failed to retrieve process memory info");
            return 0;
        }

        return (size_t)memInfo.PeakWorkingSetSize;
    #else
        LOG_WARNING("Retrieving memory usage not yet supported on platforms other than Windows");
        return 0;
    #endif
}
