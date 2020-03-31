#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    enum class EMemoryType
    {
        NONE        = 0,
        CPU_MEMORY  = 1,
        GPU_MEMORY  = 2,
    };

    enum class EFormat
    {
        NONE            = 0,
        R8G8B8A8_UNORM  = 1,
        B8G8R8A8_UNORM  = 2,
    };
}
