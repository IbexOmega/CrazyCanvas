#pragma once
#include "IEventHandler.h"

namespace LambdaEngine
{
	class EventHandler : public IEventHandler
	{
	public:
		EventHandler()	= default;
		~EventHandler() = default;

        virtual void FocusChanged(IWindow* pWindow, bool hasFocus) override
        {
            UNREFERENCED_VARIABLE(pWindow);
            UNREFERENCED_VARIABLE(hasFocus);
        }

        virtual void WindowMoved(IWindow* pWindow, int16 x, int16 y) override
        {
            UNREFERENCED_VARIABLE(pWindow);
            UNREFERENCED_VARIABLE(x);
            UNREFERENCED_VARIABLE(y);
        }
        
        virtual void WindowResized(IWindow* pWindow, uint16 width, uint16 height, EResizeType type) override
        {
            UNREFERENCED_VARIABLE(pWindow);
            UNREFERENCED_VARIABLE(width);
            UNREFERENCED_VARIABLE(height);
            UNREFERENCED_VARIABLE(type);
        }
        
        virtual void WindowClosed(IWindow* pWindow) override
        {
            UNREFERENCED_VARIABLE(pWindow);
        }
        
        virtual void MouseEntered(IWindow* pWindow) override
        {
            UNREFERENCED_VARIABLE(pWindow);
        }
        
        virtual void MouseLeft(IWindow* pWindow) override
        {
            UNREFERENCED_VARIABLE(pWindow);
        }
        
        virtual void MouseMoved(int32 x, int32 y) override
        {
            UNREFERENCED_VARIABLE(x);
            UNREFERENCED_VARIABLE(y);
        }
        
        virtual void ButtonPressed(EMouseButton button, uint32 modifierMask) override
        {
            UNREFERENCED_VARIABLE(button);
            UNREFERENCED_VARIABLE(modifierMask);
        }
        
        virtual void ButtonReleased(EMouseButton button) override
        {
            UNREFERENCED_VARIABLE(button);
        }
        
        virtual void MouseScrolled(int32 deltaX, int32 deltaY) override
        {
            UNREFERENCED_VARIABLE(deltaX);
            UNREFERENCED_VARIABLE(deltaY);
        }

        virtual void KeyPressed(EKey key, uint32 modifierMask, bool isRepeat) override
        {
            UNREFERENCED_VARIABLE(key);
            UNREFERENCED_VARIABLE(modifierMask);
            UNREFERENCED_VARIABLE(isRepeat);
        }
        
        virtual void KeyReleased(EKey key) override
        {
            UNREFERENCED_VARIABLE(key);
        }

        virtual void KeyTyped(uint32 character) override
        {
            UNREFERENCED_VARIABLE(character);
        }
	};
}