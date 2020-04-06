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

	std::string MacSocketFactory::GetLocalAddress()
	{
		return SocketFactory::GetLocalAddress();
	}
}
#endif
