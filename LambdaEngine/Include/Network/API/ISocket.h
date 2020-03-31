#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>

namespace LambdaEngine
{
	class LAMBDA_API ISocket
	{
	public:
		DECL_INTERFACE(ISocket);

		virtual bool Bind(const std::string& address, uint16 port) = 0;
		virtual void Close() = 0;
	};
}
