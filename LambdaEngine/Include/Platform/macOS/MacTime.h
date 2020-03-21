#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/Common/Time.h"

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
            s_Frequency = uint64(info.numer) / uint64(info.denom);
        }

        static FORCEINLINE uint64 Nanoseconds()
        {
            return mach_absolute_time() * s_Frequency;
        }

    private:
        inline static uint64 s_Frequency = 0;
    };

    typedef MacTime PlatformTime;
}

#endif