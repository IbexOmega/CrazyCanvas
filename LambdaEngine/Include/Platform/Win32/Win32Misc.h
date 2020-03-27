#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Common/Misc.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32Misc : public Misc
	{
	public:
		DECL_STATIC_CLASS(Win32Misc);

		static void MessageBox(const char* pCaption, const char* pText);
		static void OutputDebugString(const char*, ...);
	};

	typedef Win32Misc PlatformMisc;
}

#endif