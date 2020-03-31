#pragma once

#include "Network/API/ISocketFactory.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32SocketFactory : public ISocketFactory
	{
	public:
		virtual bool Init() override;
		virtual void Release() override;
		virtual ISocket* CreateSocket(EProtocol protocol) override;
	};
}