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

		/*
		* Getter for the current KeyboardState
		*
		* return - Returns the KeyboardState by value 
		*/
        virtual KeyboardState   GetKeyboardState()  const = 0;

		/*
		* Getter for the current MouseState
		*
		* return - Returns the MouseState by value
		*/
        virtual MouseState      GetMouseState()     const = 0;

		/*
		* Adds an IKeyboardHandler which gets called on Keyboard events
		*/
        virtual void AddKeyboardHandler(IKeyboardHandler* pHandler) = 0;

		/*
		* Adds an IMouseHandler which gets called on Mouse events
		*/
		virtual void AddMouseHandler(IMouseHandler* pHandler)       = 0;

		/*
		* Remove an IKeyboardHandler
		*/
		virtual void RemoveKeyboardHandler(IKeyboardHandler* pHandler)  = 0;

		/*
		* Remove an IMouseHandler
		*/
		virtual void RemoveMouseHandler(IMouseHandler* pHandler)        = 0;
    };
}