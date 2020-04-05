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
		virtual void OnMouseMove(int32 x, int32 y) = 0;

		/*
		* Will be called when a mouse button pressed event occurs
		*	button - Which button was pressed
		*/
		virtual void OnButtonPressed(EMouseButton button)   = 0;

		/*
		* Will be called when a mouse button released event occurs
		*	button - Which button was released
		*/
		virtual void OnButtonReleased(EMouseButton button)  = 0;

		/*
		* Will be called when a mouse scroll event occurs
		*	delta - The amount of scrolling delta < 0 for downwards scrolling and delta > 0 for upwards scrolling
		*/
		virtual void OnScroll(int32 delta) = 0;
	};
}
