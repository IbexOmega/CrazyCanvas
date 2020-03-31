#pragma once

#include "Defines.h"
#include "ISocketTCP.h"
#include "ISocketUDP.h"

namespace LambdaEngine
{
	class ISocketFactory;

	class LAMBDA_API SocketFactory
	{
		friend class EngineLoop;

	public:
		DECL_STATIC_CLASS(SocketFactory);
		static ISocketTCP* CreateSocketTCP();
		static ISocketUDP* CreateSocketUDP();

	private:
		static bool Init();

	private:
		static ISocketFactory* m_pSocketFactory;
	};
}