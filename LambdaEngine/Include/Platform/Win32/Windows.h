#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN 1 
		#include <Windows.h>
	#endif
#endif