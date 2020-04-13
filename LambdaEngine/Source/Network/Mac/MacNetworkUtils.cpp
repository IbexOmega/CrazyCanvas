#ifdef LAMBDA_PLATFORM_MACOS
#include "MacNetworkUtils.h"
#include "MacSocketTCP.h"
#include "MacSocketUDP.h"

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
		return nullptr;//new MacSocketTCP();
	}

	ISocketUDP* MacNetworkUtils::CreateSocketUDP()
	{
        return nullptr;//new MacSocketUDP();
	}

	std::string MacNetworkUtils::GetLocalAddress()
	{
		return NetworkUtils::GetLocalAddress();
	}
}
#endif
