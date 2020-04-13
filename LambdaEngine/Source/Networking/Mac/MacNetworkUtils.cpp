#ifdef LAMBDA_PLATFORM_MACOS
#include "MacNetworkUtils.h"
#include "MacSocketTCP.h"
#include "MacSocketUDP.h"
#include "MacIPAddress.h"

namespace LambdaEngine
{
	bool MacNetworkUtils::Init()
	{
		return NetworkUtils::Init();
	}

	void MacNetworkUtils::Release()
	{
		NetworkUtils::Release();
	}

	ISocketTCP* MacNetworkUtils::CreateSocketTCP()
	{
		return new MacSocketTCP();
	}

	ISocketUDP* MacNetworkUtils::CreateSocketUDP()
	{
		return new MacSocketUDP();
	}

	IPAddress* Win32NetworkUtils::CreateIPAddress(const std::string& address, uint64 hash)
	{
		return DBG_NEW MacIPAddress(address, hash);
	}
}
#endif
