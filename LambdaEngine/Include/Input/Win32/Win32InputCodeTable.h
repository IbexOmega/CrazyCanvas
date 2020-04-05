#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/API/InputCodes.h"

namespace LambdaEngine
{
	class Win32InputCodeTable
	{
	public:
		DECL_STATIC_CLASS(Win32InputCodeTable);

		/*
		* Initializes KeyCode- table
		*
		* return - Returns true if successful
		*/
		static bool Init();

		/*
		* Returns a LambdaEngine- EKey from a Win32- virtualkey
		*
		* keyCode - A windows virtualkey to be converted. See https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
		*
		* return - Returns a keycode of type EKey
		*/
		static EKey GetKey(int32 keyCode);

	private:
		static EKey s_KeyCodeTable[EKey::KEY_COUNT];
	};
}
#endif