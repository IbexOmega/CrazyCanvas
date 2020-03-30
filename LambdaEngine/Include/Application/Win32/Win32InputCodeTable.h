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

	private:
		static EKey 		s_KeyCodeTable[EKey::KEY_COUNT];
	};
}
#endif