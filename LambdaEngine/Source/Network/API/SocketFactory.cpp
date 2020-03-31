#include "Network/API/SocketFactory.h"
#include "Log/Log.h"

#ifdef LAMBDA_PLATFORM_WINDOWS
    #include "Network/Win32/Win32SocketFactory.h"
#else
    #include "Network/API/ISocketFactory.h"
#endif

namespace LambdaEngine
{
	ISocketFactory* SocketFactory::m_pSocketFactory = nullptr;

	ISocketTCP* SocketFactory::CreateSocketTCP()
	{
		return m_pSocketFactory->CreateSocketTCP();
	}

	ISocketUDP* SocketFactory::CreateSocketUDP()
	{
		return m_pSocketFactory->CreateSocketUDP();
	}

	bool SocketFactory::Init()
	{
		if (m_pSocketFactory)
		{
			LOG_ERROR("[SocketFactory] has already been initialized!");
			return false;
		}

#ifdef LAMBDA_PLATFORM_WINDOWS
		m_pSocketFactory = new Win32SocketFactory();
#endif
		if (m_pSocketFactory)
		{
			return m_pSocketFactory->Init();
		}

		return false;
	}
}
