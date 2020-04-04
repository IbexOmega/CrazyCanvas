#pragma once
#include "InputCodes.h"

namespace LambdaEngine
{
    struct KeyboardState
	{
		bool KeyStates[EKey::KEY_LAST];

		bool IsKeyDown(EKey key)
		{
			return KeyStates[key];
		}

		bool IsKeyUp(EKey key)
		{
			return !KeyStates[key];
		}
	};

	struct MouseState
	{
		int32 x;
		int32 y;
		int32 Scroll;

		bool ButtonStates[EMouseButton::MOUSE_BUTTON_COUNT];

		bool IsButtonPressed(EMouseButton button)
		{
			return ButtonStates[button];
		}

		bool IsButtonReleased(EMouseButton button)
		{
			return !ButtonStates[button];
		}
	};
}