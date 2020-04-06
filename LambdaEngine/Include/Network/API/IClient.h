#pragma once
#include "Defines.h"
#include "Types.h"
#include <string>

namespace LambdaEngine
{
	class NetworkPacket;

	class LAMBDA_API IClient
	{
	public:
		DECL_INTERFACE(IClient);

		/*
		* Sends a packet
		*/
		virtual bool SendPacket(NetworkPacket* packet) = 0;

		/*
		* Sends a packet Immediately using the current thread
		*/
		virtual bool SendPacketImmediately(NetworkPacket* packet) = 0;

		/*
		* Release all the resouces used by the client and will be deleted when each thread has terminated.
		*/
		virtual void Release() = 0;

		/*
		* return - true if this instance is on the server side, otherwise false.
		*/
		virtual bool IsServerSide() const = 0;

		/*
		* return - The currently used inet address.
		*/
		virtual const std::string& GetAddress() const = 0;

		/*
		* return - The currently used port.
		*/
		virtual uint16 GetPort() const = 0;

		/*
		* return - The total number of bytes sent
		*/
		virtual int32 GetBytesSent() const = 0;

		/*
		* return - The total number of bytes received
		*/
		virtual int32 GetBytesReceived() const = 0;

		/*
		* return - The total number of packets sent
		*/
		virtual int32 GetPacketsSent() const = 0;

		/*
		* return - The total number of packets received
		*/
		virtual int32 GetPacketsReceived() const = 0;
	};
}
