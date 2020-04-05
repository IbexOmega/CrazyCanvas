#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    class Window
    {
    public:       
		DECL_ABSTRACT_CLASS(Window);
        
        virtual bool Init(const char* pTitle, uint32 width, uint32 height)  = 0;
        
        virtual void Show() = 0;
        
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
    };
}
