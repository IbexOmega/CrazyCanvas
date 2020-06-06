#pragma once
#include "Core/RefCountedObject.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	enum FWindowStyleFlags : uint32
	{
		WINDOW_STYLE_FLAG_NONE 			= 0,
		WINDOW_STYLE_FLAG_TITLED 		= FLAG(1),
		WINDOW_STYLE_FLAG_CLOSABLE 		= FLAG(2),
		WINDOW_STYLE_FLAG_MINIMIZABLE 	= FLAG(3),
		WINDOW_STYLE_FLAG_MAXIMIZABLE   = FLAG(4),
		WINDOW_STYLE_FLAG_RESIZEABLE 	= FLAG(5),
	};

	struct WindowDesc
	{
		String Title	= "";
		uint16 Width 	= 0;
		uint16 Height	= 0;
		uint32 Style	= 0;
	};

	class Window : public RefCountedObject
	{
	public:
		DECL_ABSTRACT_CLASS(Window);
		
		virtual void Show()   = 0;
		virtual void Close()  = 0;
		
		virtual void Minimize() = 0;
		virtual void Maximize() = 0;
		
		/*
		* Returns true if this is the current active window, that is the window that currently has input focus
		*	return - Returns true if the window is the current active window
		*/
		virtual bool IsActiveWindow() const = 0;

		/*
		* Restores a window from minimized or maximized state
		*/
		virtual void Restore() = 0;
		
		virtual void ToggleFullscreen() = 0;

		virtual void SetTitle(const String& title) = 0;
		
		virtual void SetPosition(int32 x, int32 y)					= 0;
		virtual void GetPosition(int32* pPosX, int32* pPosY) const	= 0;

		/*
		* Retrive the native handle
		*	return - Return the native handle of the window. 
		*				Win32:	HWND
		*				Mac:	NSWindow*
		*/
		virtual void* GetHandle() const
		{
			return nullptr;
		}

		/*
		* Return a handle to the client area of the window. May be nullptr on some platforms
		*	return - Returns a handle to the client-area of the window
		*		Win32:	nullptr
		*		Mac:	NSView*
		*/
		virtual const void* GetView() const 
		{ 
			return nullptr; 
		}

		virtual void SetSize(uint16 width, uint16 height) = 0;
		
		virtual uint16 GetWidth()  const 
		{ 
			return 0; 
		}

		virtual uint16 GetHeight() const 
		{ 
			return 0; 
		}
		
		/*
		* Returns the scale of the client area. This is necessary for High resolution displays scales the resolution.
		*   return - Floating point scale of client area, assumes that the scaling is uniform
		*/
		virtual float32 GetClientAreaScale() const 
		{ 
			return 1.0f; 
		}

	protected:
		WindowDesc m_Desc;
	};
}
