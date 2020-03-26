#pragma once
#include "LambdaEngine.h"

#ifdef LAMBDA_PLATFORM_WINDOWS
    #include "Platform/Win32/Windows.h"
#endif

namespace LambdaEngine
{
    class IApplicationMessageHandler
    {
    public:
        DECL_INTERFACE(IApplicationMessageHandler);
        
#ifdef LAMBDA_PLATFORM_WINDOWS
        virtual LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) = 0;
#endif
    };
}
