#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/API/InputCodes.h"

#define NUM_KEY_CODES 512

namespace LambdaEngine
{
	class Win32InputCodeTable
	{
	public:
		DECL_STATIC_CLASS(Win32InputCodeTable);

		/*
		* Initializes KeyCode- table
		*	return - Returns true if successful
		*/
		static bool Init();

		/*
		* Returns a LambdaEngine::EKey from a Win32- scancode
		*	keyCode	- A windows scancode to be converted. Should not be confused with virtual keys
		*	return	- Returns a keycode of type EKey
		*/
		static EKey GetKeyFromScanCode(uint32 scanCode);

		/*
		* Returns a LambdaEngine::EKey from a Win32- virtualkey
		*	keyCode	- A windows virtualkey to be converted. See https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
		*	return	- Returns a keycode of type EKey
		*/
		static EKey GetKeyFromVirtualKey(uint32 virtualKey);

		/*
		* Returns a Win32- scancode from a LambdaEngine::EKey
		*	key		- A  LambdaEngine::EKey to be converted.
		*	return	- Returns a scancode of type uint16
		*/
		static uint16 GetScanCodeFromKey(EKey key);

		/*
		* Returns a Win32- virtualkey from a LambdaEngine::EKey. This function does not distinguish between left- and right-hand keys.
		*	key		- A  LambdaEngine::EKey to be converted.
		*	return	- Returns a virtualkey of type uint16. See https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
		*/
		static uint16 GetVirtualKeyFromKey(EKey key);

		/*
		* Returns the current modifermask
		*	return - A mask of modiferFlags
		*/
		static uint32 GetModifierMask();

	private:
		static EKey		s_KeyCodeTable[NUM_KEY_CODES];
		static uint16	s_ScanCodeTable[NUM_KEY_CODES];
	};
}
#endif