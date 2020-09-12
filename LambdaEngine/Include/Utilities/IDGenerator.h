#pragma once

#include "Types.h"

#include <queue>

namespace LambdaEngine
{
    class IDGenerator
    {
    public:
        IDGenerator();
        ~IDGenerator() = default;

        uint32 GenID();

        // Registers ID as free to be generated again
        void PopID(uint32 ID);

    private:
        uint32 m_NextFree;

        std::queue<uint32_t> m_Recycled;
    };
}
