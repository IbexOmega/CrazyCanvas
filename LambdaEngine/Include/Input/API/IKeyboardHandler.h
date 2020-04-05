#pragma once
#include "InputCodes.h"

namespace LambdaEngine
{
	class IKeyboardHandler
	{
	public:
		DECL_INTERFACE(IKeyboardHandler);
		
		/*
		* Will be called when a key pressed event occurs
		*	key - Which key was pressed
		*/
		virtual void OnKeyDown(EKey key)		= 0;

		/*
		* Will be called once for each event that occurs when a key is continually held down
		*	key - Which key is held down
		*/
		virtual void OnKeyHeldDown(EKey key)	= 0;

		/*
		* Will be called when a key released event occurs
		*	key - Which key was released
		*/
		virtual void OnKeyUp(EKey key)			= 0;
	};
}
