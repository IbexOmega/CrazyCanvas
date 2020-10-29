#pragma once
#include "Containers/String.h"

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

		#ifdef CreateWindow
			#undef CreateWindow
		#endif

		#ifdef CreateProcess
			#undef CreateProcess
		#endif

		#ifdef ERROR
			#undef ERROR
		#endif

		#ifdef CreateWindow
			#undef CreateWindow
		#endif

		#ifdef UpdateResource
			#undef UpdateResource
		#endif

		#ifdef FindResource
			#undef FindResource
		#endif

		#ifdef UNREFERENCED_PARAMETER
			#undef UNREFERENCED_PARAMETER
		#endif

		#ifdef RELATIVE
			#undef RELATIVE
		#endif
		#ifdef CreateProcess
			#undef CreateProcess
		#endif
	#endif

namespace LambdaEngine
{
	inline String GetLastErrorAsString()
	{
		DWORD dwError = ::GetLastError();
		if (dwError == 0)
		{
			return String();
		}

		LPSTR messageBuffer = nullptr;
		size_t size = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, NULL);

		String message(messageBuffer, size);
		// Free the buffer 
		LocalFree(messageBuffer);
		return message;
	}
}

#endif