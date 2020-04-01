#pragma once

#include "ISocketTCP.h"
#include "ISocketUDP.h"

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Network/Win32/Win32SocketFactory.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
    #include "Network/Mac/MacSocketFactory.h"
#else
	#error No platform defined
#endif
