#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/InputCodes.h"

namespace LambdaEngine
{
	class Win32InputCodeTable
	{
	public:
		DECL_STATIC_CLASS(Win32InputCodeTable);

		static bool Init();

		static EKey 		GetKey(int32 keyCode);
		static EMouseButton GetMouseButton(int32 mouseButtonCode);

	private:
		static EKey 		s_KeyCodeTable[EKey::KEY_COUNT];
		static EMouseButton s_MouseButtonCodeTable[EMouseButton::MOUSE_BUTTON_COUNT];
	};
}
#endif