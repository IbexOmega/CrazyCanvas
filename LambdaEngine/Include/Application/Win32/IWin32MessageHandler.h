#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "LambdaEngine.h"

#include "Application/Win32/Windows.h"

namespace LambdaEngine
{
    class IWin32MessageHandler
    {
    public:
        DECL_INTERFACE(IWin32MessageHandler);

        /*
        * Handles messages sent from the application's messageproc
        * See https://docs.microsoft.com/en-us/windows/win32/learnwin32/writing-the-window-procedure
        */
        virtual LRESULT ProcessMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;
    };
}

#endif