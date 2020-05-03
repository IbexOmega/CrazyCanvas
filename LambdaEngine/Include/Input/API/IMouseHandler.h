#pragma once
#include "InputCodes.h"

namespace LambdaEngine
{
	class IMouseHandler
	{
	public:
		DECL_INTERFACE(IMouseHandler);

		/*
		* Will be called when a mouse move event occurs
		*	x - The new horizontal window coordinates of the mouse
		*	y - The new vertices window coordinates of the mouse
		*/
		virtual void MouseMoved(int32 x, int32 y) = 0;

		/*
		* Will be called when a mouse button pressed event occurs
		*	button - Which button was pressed
		*/
		virtual void ButtonPressed(EMouseButton button, uint32 modifierMask) = 0;

		/*
		* Will be called when a mouse button released event occurs
		*	button - Which button was released
		*/
        virtual void ButtonReleased(EMouseButton button) = 0;

		/*
		* Will be called when a mouse scroll event occurs
        *   deltaX - The amount of scrolling delta < 0 for left scrolling and delta > 0 for right scrolling
        *   deltaY - The amount of scrolling delta < 0 for downwards scrolling and delta > 0 for upwards scrolling
		*/
		virtual void MouseScrolled(int32 deltaX, int32 deltaY) = 0;
	};
}
