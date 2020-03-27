#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN 1
		#define NOMINMAX 1
		#include <Windows.h>

		#ifdef MessageBox
			#undef MessageBox
		#endif
		#ifdef OutputDebugString
			#undef OutputDebugString
		#endif
		#ifdef ERROR
			#undef ERROR
		#endif
	#endif
#endif