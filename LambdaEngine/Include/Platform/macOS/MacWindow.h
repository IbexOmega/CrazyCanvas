#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/Common/Window.h"

#include "MacWindowDelegate.h"

#ifdef __OBJC__
    #include <Appkit/Appkit.h>
#else
    class NSWindow;
#endif

namespace LambdaEngine
{
    class MacWindow : public Window
    {
    public:
        MacWindow()     = default;
        ~MacWindow()    = default;

        DECL_REMOVE_COPY(MacWindow);
        DECL_REMOVE_MOVE(MacWindow);

        virtual bool Init(uint32 width, uint32 height) override;
        virtual void Release() override;

        virtual void Show() override;

        FORCEINLINE virtual void* GetHandle() const override
        {
            return (void*)m_pWindow;
        }
        
    private:
        NSWindow*           m_pWindow   = nullptr;
        MacWindowDelegate*  m_pDelegate = nullptr;
    };
}

#endif
