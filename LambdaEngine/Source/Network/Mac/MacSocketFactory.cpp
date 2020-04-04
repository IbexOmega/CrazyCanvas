#ifdef LAMBDA_PLATFORM_MACOS
#include "MacSocketFactory.h"
#include "MacSocketTCP.h"
#include "MacSocketUDP.h"

namespace LambdaEngine
{
	bool MacSocketFactory::Init()
	{
		return SocketFactory::Init();
	}

	void MacSocketFactory::Release()
	{
		SocketFactory::Release();
	}

	ISocketTCP* MacSocketFactory::CreateSocketTCP()
	{
		return new MacSocketTCP();
	}

	ISocketUDP* MacSocketFactory::CreateSocketUDP()
	{
		return new MacSocketUDP();
	}

	const std::string& MacSocketFactory::GetLocalAddress()
	{
		ISocketUDP* socketUDP = CreateSocketUDP();
		if (socketUDP)
		{
			if (socketUDP->Connect(ADDRESS_LOOPBACK, 9))
			{
				return socketUDP->GetAddress();
			}
		}
		return ADDRESS_LOOPBACK;
	}
}
#endif
