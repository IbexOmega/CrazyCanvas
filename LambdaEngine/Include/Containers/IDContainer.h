#pragma once

#include "Containers/TArray.h"
#include "Types.h"

namespace LambdaEngine
{
    class IDContainer
    {
    public:
        virtual bool HasElement(uint32 ID) const = 0;

        virtual uint32 Size() const = 0;
        virtual const TArray<uint32>& GetIDs() const = 0;
        virtual void Pop(uint32 ID) = 0;
    };
}
