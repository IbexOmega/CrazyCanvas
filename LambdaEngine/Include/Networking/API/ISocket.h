#pragma once
#include "Defines.h"
#include "Types.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	class IPEndPoint;

	class ISocket
	{
	public:
		DECL_INTERFACE(ISocket);

		/*
		* Binds the socket to a given ip-address and port. To bind a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* pIPEndPoint - The IPEndPoint to bind the socket to
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Bind(const IPEndPoint& pIPEndPoint) = 0;

		/*
		* Connects the socket to a given ip-address and port. To connect to a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* pIPEndPoint - The IPEndPoint to connect the socket to
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Connect(const IPEndPoint& pIPEndPoint) = 0;

		/*
		* Sets the socket in non blocking or blocking mode.
		*
		* enable - True to use blocking calls, false for non blocking calls.
		*
		* return - False if an error occured, otherwise true.
		*/
		virtual bool EnableBlocking(bool enable) = 0;
		virtual bool IsNonBlocking() const = 0;

		/*
		* Closes the socket
		*
		* return - False if an error occured, otherwise true.
		*/
		virtual bool Close() = 0;
		virtual bool IsClosed() const = 0;

		/*
		* return - The IPEndPoint currently Bound or Connected to
		*/
		virtual const IPEndPoint& GetEndPoint() const = 0;
	};
}
