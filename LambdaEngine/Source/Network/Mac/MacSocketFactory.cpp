#ifdef LAMBDA_PLATFORM_MACOS
#include "MacSocketFactory.h"
#include "MacSocketTCP.h"
#include "MacSocketUDP.h"

namespace LambdaEngine
{
	bool MacSocketFactory::Init()
	{
		return true;
	}

	void MacSocketFactory::Release()
	{
		
	}

	ISocketTCP* MacSocketFactory::CreateSocketTCP()
	{
		return new MacSocketTCP();
	}

	ISocketUDP* MacSocketFactory::CreateSocketUDP()
	{
		return new MacSocketUDP();
	}
}
#endif
