#pragma once
#include "InputCodes.h"

namespace LambdaEngine
{
	/*
	* KeyboardState
	*/
	struct KeyboardState
	{
	public:
		inline bool IsKeyDown(EKey key) const
		{
			return KeyStates[key];
		}

		inline bool IsKeyUp(EKey key) const
		{
			return !KeyStates[key];
		}
		
	public:
		bool KeyStates[EKey::KEY_LAST];
	};

	/*
	* ModiferKeyState
	*/
	struct ModifierKeyState
	{
	public:
		inline ModifierKeyState()
			: ModiferMask(0)
		{
		}

		inline explicit ModifierKeyState(uint32 modiferMask)
			: ModiferMask(modiferMask)
		{
		}

		inline bool IsShiftDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_SHIFT);
		}

		inline bool IsAltDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_ALT);
		}

		inline bool IsCtrlDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_CTRL);
		}

		inline bool IsSuperDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_SUPER);
		}

		inline bool IsCapsLockDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_CAPS_LOCK);
		}

		inline bool IsNumLockDown() const
		{
			return (ModiferMask & FModifierFlag::MODIFIER_FLAG_NUM_LOCK);
		}

	public:
		uint32 ModiferMask;
	};

	/*
	* MouseState
	*/
	struct MouseState
	{
	public:
		inline bool IsButtonPressed(EMouseButton button) const
		{
			return ButtonStates[button];
		}

		inline bool IsButtonReleased(EMouseButton button) const
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
