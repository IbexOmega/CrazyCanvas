#pragma once

#include "Networking/API/TCP/ISocketTCP.h"
#include "Networking/API/UDP/ISocketUDP.h"

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Networking/Win32/Win32NetworkUtils.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
    #include "Networking/Mac/MacNetworkUtils.h"
#else
	#error No platform defined
#endif
