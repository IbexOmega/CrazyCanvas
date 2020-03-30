#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    class LAMBDA_API IDeviceChild
    {
    public:
        DECL_INTERFACE(IDeviceChild);

        virtual uint64 Release()    = 0;
        virtual uint64 AddRef()     = 0;
        
        virtual void SetName(const char* pName) = 0;
    };
}
