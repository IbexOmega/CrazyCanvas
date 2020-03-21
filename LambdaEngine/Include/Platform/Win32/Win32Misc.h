#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Common/Misc.h"

#ifdef MessageBox
	#undef MessageBox
#endif

namespace LambdaEngine
{
	class LAMBDA_API Win32Misc : public Misc
	{
	public:
		DECL_STATIC_CLASS(Win32Misc);

		static void MessageBox(const char* pTitle, const char* pText);
	};

	typedef Win32Misc PlatformMisc;
}

#endif