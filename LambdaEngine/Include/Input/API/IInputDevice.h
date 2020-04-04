#pragma once
#include "Application/API/IApplicationMessageHandler.h"
#include "InputState.h"

namespace LambdaEngine
{
    class IMouseHandler;
	class IKeyboardHandler;

    class IInputDevice : public IApplicationMessageHandler
    {
    public:
        DECL_INTERFACE(IInputDevice);

        virtual KeyboardState   GetKeyboardState()  const = 0;
        virtual MouseState      GetMouseState()     const = 0;

        virtual void AddKeyboardHandler(IKeyboardHandler* pHandler) = 0;
		virtual void AddMouseHandler(IMouseHandler* pHandler)       = 0;

		virtual void RemoveKeyboardHandler(IKeyboardHandler* pHandler)  = 0;
		virtual void RemoveMouseHandler(IMouseHandler* pHandler)        = 0;
    };
}