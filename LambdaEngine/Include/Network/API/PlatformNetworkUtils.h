#pragma once

#include "TCP/ISocketTCP.h"
#include "UDP/ISocketUDP.h"

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Network/Win32/Win32NetworkUtils.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
    #include "Network/Mac/MacNetworkUtils.h"
#else
	#error No platform defined
#endif
