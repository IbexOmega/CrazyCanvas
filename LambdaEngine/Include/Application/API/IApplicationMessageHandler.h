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
        /*
        * Handles messages sent from the application's messageproc
        * See https://docs.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
        */
        virtual LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) = 0;
#elif defined(LAMBDA_PLATFORM_MACOS)

        /*
        * Handles events sent to the application
        *
        * event - The event sent from the application. Event should NOT be released by the implementation of HandleEvent.
        */
        virtual void HandleEvent(NSEvent* event) = 0;
#endif
    };
}
