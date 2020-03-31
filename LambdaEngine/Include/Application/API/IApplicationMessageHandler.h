#pragma once
#include "LambdaEngine.h"

#ifdef LAMBDA_PLATFORM_WINDOWS
    #include "Application/Win32/Windows.h"
#endif

#ifdef __OBJC__
@class NSEvent;
#else
class NSEvent;
#endif

namespace LambdaEngine
{
    class IApplicationMessageHandler
    {
    public:
        DECL_INTERFACE(IApplicationMessageHandler);
        
#if defined(LAMBDA_PLATFORM_WINDOWS)
        virtual LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) = 0;
#elif defined(LAMBDA_PLATFORM_MACOS)
        virtual void HandleEvent(NSEvent* event) = 0;
#endif
    };
}
