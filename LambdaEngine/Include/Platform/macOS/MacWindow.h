#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "LambdaEngine.h"

#include "MacWindowDelegate.h"

#ifdef __OBJC__
    #include <Appkit/Appkit.h>
#else
    class NSWindow;
#endif

namespace LambdaEngine
{
    class MacWindow
    {
    public:
        MacWindow();
        ~MacWindow() = default;

        DECL_REMOVE_COPY(MacWindow);
        DECL_REMOVE_MOVE(MacWindow);

        bool Init(uint32 width, uint32 height);
        void Show();
        void Release();

    private:
        NSWindow*           m_Window;
        MacWindowDelegate*  m_Delegate;
    };
}

#endif
