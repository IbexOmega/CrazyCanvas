#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Application/API/Window.h"

#ifdef __OBJC__
@class CocoaWindow;
@class CocoaContentView;
#else
class CocoaWindow;
class CocoaContentView;
#endif

namespace LambdaEngine
{
    class MacWindow : public Window
    {
    public:
        MacWindow();
        ~MacWindow();

        bool Init(const WindowDesc* pDesc);
        
        // IWindow interface
        virtual void Show()     override final;
        virtual void Close()    override final;
        
        virtual void Minimize() override final;
        virtual void Maximize() override final;
        
		virtual bool IsActiveWindow() const override final;
		
        virtual void Restore() override final;
        
        virtual void ToggleFullscreen() override final;

        virtual void SetTitle(const String& title) override final;
        
		virtual void SetPosition(int32 x, int32 y)                  override final;
		virtual void GetPosition(int32* pPosX, int32* pPosY) const  override final;
		
		virtual void*       GetHandle() const override final;
		virtual const void* GetView()   const override final;
		
		virtual void SetSize(uint16 width, uint16 height) override final;
		
        virtual uint16 GetWidth()  const override final;
        virtual uint16 GetHeight() const override final;
        
		virtual float32 GetClientAreaScale() const override final;
		
    private:
        CocoaWindow*            m_pWindow   = nullptr;
        CocoaContentView*       m_pView     = nullptr;
		
		uint32 m_StyleFlags = 0;
    };
}

#endif
