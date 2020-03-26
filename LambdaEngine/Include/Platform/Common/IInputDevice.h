#pragma once

#include "IApplicationMessageHandler.h"

namespace LambdaEngine
{
	struct KeyboardState
	{
		bool KeyStates[256];

		bool IsKeyDown(uint32 key)
		{
			return KeyStates[key];
		}

		bool IsKeyUp(uint32 key)
		{
			return !KeyStates[key];
		}
	};

	struct MouseState
	{
		int32 X;
		int32 Y;
		int32 Scroll;

		bool ButtonStates[5];

		bool IsButtonPressed(uint32 button)
		{
			return ButtonStates[button];
		}

		bool IsButtonReleased(uint32 button)
		{
			return !ButtonStates[button];
		}
	};

	class IInputDevice : public IApplicationMessageHandler
	{
	public:
		DECL_INTERFACE(IInputDevice);

		virtual KeyboardState GetKeyboardState() = 0;
		virtual MouseState GetMouseState() = 0;
	};
}