#pragma once
#include "InputCodes.h"

namespace LambdaEngine
{
    struct KeyboardState
	{
    public:
        bool IsKeyDown(EKey key) const
        {
            return KeyStates[key];
        }

        bool IsKeyUp(EKey key) const
        {
            return !KeyStates[key];
        }
        
    public:
        bool KeyStates[EKey::KEY_LAST];
	};

	struct MouseState
	{
    public:
        bool IsButtonPressed(EMouseButton button) const
        {
            return ButtonStates[button];
        }

        bool IsButtonReleased(EMouseButton button) const
        {
            return !ButtonStates[button];
        }
        
    public:
        int32 x;
        int32 y;
        int32 ScrollX;
        int32 ScrollY;
        
        bool ButtonStates[EMouseButton::MOUSE_BUTTON_COUNT];
	};
}
