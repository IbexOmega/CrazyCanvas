#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Time/API/Time.h"

#include <mach/mach_time.h>

namespace LambdaEngine
{
    class MacTime : public Time
    {
    public:
        DECL_STATIC_CLASS(MacTime);

        static FORCEINLINE void PreInit()
        {
            mach_timebase_info_data_t info = {};
            mach_timebase_info(&info);
            
            constexpr uint64 NANOSECONDS = 1000 * 1000 * 1000;
            s_Frequency = ((NANOSECONDS* uint64(info.numer)) / uint64(info.denom));
        }
        
        static FORCEINLINE uint64 GetPerformanceCounter()
        {
            return mach_absolute_time();
        }

        static FORCEINLINE uint64 GetPerformanceFrequency()
        {
            return s_Frequency;
        }

    private:
        inline static uint64 s_Frequency = 0;
    };

    typedef MacTime PlatformTime;
}

#endif
