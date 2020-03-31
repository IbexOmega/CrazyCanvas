#pragma once

#include "Defines.h"
#include "EProtocol.h"
#include "ISocket.h"

namespace LambdaEngine
{
	class ISocket;
	class ISocketFactory;

	class LAMBDA_API SocketFactory
	{
		friend class EngineLoop;

	public:
		DECL_STATIC_CLASS(SocketFactory);
		static ISocket* CreateSocket(EProtocol protocol);

	private:
		static bool Init();

	private:
		static ISocketFactory* m_pSocketFactory;
	};
}