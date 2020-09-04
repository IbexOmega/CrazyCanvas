#pragma once

#include "Defines.h"

#include <chrono>
#include <stdint.h>

class RuntimeStats
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
