#pragma once
#include "Defines.h"
#include "Types.h"

#include <stdarg.h>

namespace LambdaEngine
{
	class Misc
	{
	public:
		DECL_STATIC_CLASS(Misc);

		static void MessageBox(const char*, const char*)		{ }

        static void OutputDebugString(const char*, ...)			{ }
        static void OutputDebugStringV(const char*, va_list)	{ }
	};
}
