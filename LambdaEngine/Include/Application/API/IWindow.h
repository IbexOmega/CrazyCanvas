#pragma once
#include "Core/RefCountedObject.h"

namespace LambdaEngine
{
	enum FWindowStyleFlags : uint32
	{
		WINDOW_STYLE_FLAG_NONE 			= 0,
		WINDOW_STYLE_FLAG_TITLED 		= FLAG(1),
		WINDOW_STYLE_FLAG_CLOSABLE 		= FLAG(2),
		WINDOW_STYLE_FLAG_MINIMIZABLE 	= FLAG(3),
		WINDOW_STYLE_FLAG_RESIZEABLE 	= FLAG(4),
	};

	struct WindowDesc
	{
		const char* pTitle 	= "";
		uint16 		Width 	= 0;
		uint16		Height	= 0;
		uint32		Style	= 0;
	};

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
