#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "LambdaEngine.h"

#include <Foundation/Foundation.h>

#define SCOPED_AUTORELEASE_POOL(...) LambdaEngine::MacScopedPool()

namespace LambdaEngine
{
    class MacScopedPool
    {
    public:
        FORCEINLINE MacScopedPool()
        {
            NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
            m_pPool = pool;
        }
        
        FORCEINLINE ~MacScopedPool()
        {
            [m_pPool release];
        }
        
    private:
        NSAutoreleasePool* m_pPool = nullptr;
    };
}

#endif
