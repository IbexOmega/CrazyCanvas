#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/Common/Window.h"

#ifdef __OBJC__

@class CocoaWindow;
@class CocoaWindowDelegate;

#else

class CocoaWindow;
class CocoaWindowDelegate;

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

        virtual bool Init(uint32 width, uint32 height)  override final;
        virtual void Release()                          override final;

        virtual void Show() override final;

        FORCEINLINE virtual void* GetHandle() const override final
        {
            return (void*)m_pWindow;
        }
        
    private:
        CocoaWindow*          m_pWindow   = nullptr;
        CocoaWindowDelegate*  m_pDelegate = nullptr;
    };
}

#endif
