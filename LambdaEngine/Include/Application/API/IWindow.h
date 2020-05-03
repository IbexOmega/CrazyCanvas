#pragma once
#include "Core/RefCountedObject.h"

namespace LambdaEngine
{
    class IWindow : public RefCountedObject
    {
    public:       
		DECL_ABSTRACT_CLASS(IWindow);
        
        virtual void Show()   = 0;
        virtual void Close()  = 0;
        
        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        
        virtual void Restore() = 0;
        
        virtual void ToggleFullscreen() = 0;

        virtual void SetTitle(const char* pTitle) = 0;
        
        /*
        * Retrive the native handle
        * 
        * return -  Return the native handle of the window. 
        *           Win32:  HWND
        *           Mac:    NSWindow*
        */
        virtual void* GetHandle() const = 0;

        /*
        * Return a handle to the client area of the window. May be nullptr on some platforms
        * 
        * return -  Returns a handle to the client-area of the window
        *           Win32:  nullptr
        *           Mac:    NSView*
        */
        virtual const void* GetView() const = 0;

        virtual uint16 GetWidth()  const = 0;
        virtual uint16 GetHeight() const = 0;
    };
}
