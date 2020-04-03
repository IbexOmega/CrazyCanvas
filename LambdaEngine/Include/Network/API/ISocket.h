#pragma once
#include "Defines.h"
#include "Types.h"

#include <string>

namespace LambdaEngine
{
	class ISocket
	{
	public:
		DECL_INTERFACE(ISocket);

		virtual bool Bind(const std::string& address, uint16 port) = 0;
		virtual bool SetNonBlocking(bool nonBlocking) = 0;
		virtual bool IsNonBlocking() const = 0;
		virtual bool Close() = 0;
		virtual bool IsClosed() const = 0;
	};
}
