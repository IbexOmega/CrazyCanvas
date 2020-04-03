#pragma once
#include "Defines.h"
#include "Types.h"

#include <string>

#define ADDRESS_LOOPBACK	"ADDRESS_LOOPBACK"
#define ADDRESS_ANY			"ADDRESS_ANY"
#define ADDRESS_BROADCAST	"ADDRESS_BROADCAST"

namespace LambdaEngine
{
	class ISocket
	{
	public:
		DECL_INTERFACE(ISocket);

		virtual bool Bind(const std::string& address, uint16 port) = 0;
		virtual bool Connect(const std::string& address, uint16 port) = 0;
		virtual bool SetNonBlocking(bool nonBlocking) = 0;
		virtual bool IsNonBlocking() const = 0;
		virtual bool Close() = 0;
		virtual bool IsClosed() const = 0;
		virtual const std::string& GetAddress() const = 0;
		virtual uint16 GetPort() const = 0;
	};
}
