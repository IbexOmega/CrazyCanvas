#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Application/API/Window.h"

#ifdef __OBJC__

@class CocoaWindow;
@class CocoaContentView;
@class CocoaWindowDelegate;

#else

class CocoaWindow;
class CocoaContentView;
class CocoaWindowDelegate;

#endif

namespace LambdaEngine
{
    class MacWindow : public Window
    {
    public:
        MacWindow()     = default;
        ~MacWindow()    = default;

        virtual bool Init(uint32 width, uint32 height)  override final;
        virtual void Release()                          override final;

        virtual void Show() override final;

        FORCEINLINE virtual void* GetHandle() const override final
        {
            return (void*)m_pWindow;
        }

        FORCEINLINE virtual const void* GetView() const override final
        {
            return m_pView;
        }
        
    private:
        CocoaWindow*            m_pWindow   = nullptr;
        CocoaContentView*       m_pView     = nullptr;
        CocoaWindowDelegate*    m_pDelegate = nullptr;
    };
}

#endif
