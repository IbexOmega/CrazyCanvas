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
}
