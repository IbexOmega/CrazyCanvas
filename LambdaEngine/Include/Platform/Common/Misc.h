#pragma once
#include "Defines.h"
#include "Types.h"

namespace LambdaEngine
{
	class Misc
	{
	public:
		DECL_STATIC_CLASS(Misc);

		static void MessageBox(const char*, const char*)    		{ }
        static void OutputDebugString(const char* pFormat, ...)     { }
	};
}
