#pragma once

#include "Input/InputCodes.h"

#include "IApplicationMessageHandler.h"

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
		int32 X;
		int32 Y;
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

	class IInputDevice : public IApplicationMessageHandler
	{
	public:
		DECL_INTERFACE(IInputDevice);

		virtual KeyboardState GetKeyboardState() = 0;
		virtual MouseState GetMouseState() = 0;
	};
}