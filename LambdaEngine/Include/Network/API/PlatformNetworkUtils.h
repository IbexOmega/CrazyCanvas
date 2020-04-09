#pragma once

#include "TCP/ISocketTCP.h"
#include "TCP/IClientTCP.h"
#include "TCP/IClientTCPHandler.h"
#include "TCP/ServerTCP.h"
#include "TCP/IServerTCPHandler.h"

#include "UDP/ISocketUDP.h"
#include "UDP/IClientUDP.h"
#include "UDP/IClientUDPHandler.h"
#include "UDP/ServerUDP.h"
#include "UDP/IServerUDPHandler.h"

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Network/Win32/Win32NetworkUtils.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
    #include "Network/Mac/MacNetworkUtils.h"
#else
	#error No platform defined
#endif
