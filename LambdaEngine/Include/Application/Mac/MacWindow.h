#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Application/API/IWindow.h"

#ifdef __OBJC__
@class CocoaWindow;
@class CocoaContentView;
#else
class CocoaWindow;
class CocoaContentView;
#endif

namespace LambdaEngine
{
    class MacWindow : public IWindow
    {
    public:
        MacWindow();
        ~MacWindow();

        // IWindow interface
        virtual bool Init(const char* pTitle, uint32 width, uint32 height)  override final;
        
        virtual void Show()     override final;
        virtual void Close()    override final;
        
        virtual void Minimize() override final;
        virtual void Maximize() override final;
        
        virtual void Restore() override final;
        
        virtual void ToggleFullscreen() override final;

        virtual void SetTitle(const char* pTitle) override final;
        
        virtual uint16      GetWidth()  const override final;
        virtual uint16      GetHeight() const override final;
        virtual void*       GetHandle() const override final;
        virtual const void* GetView()   const override final;
        
    private:
        CocoaWindow*            m_pWindow   = nullptr;
        CocoaContentView*       m_pView     = nullptr;
    };
}

#endif
