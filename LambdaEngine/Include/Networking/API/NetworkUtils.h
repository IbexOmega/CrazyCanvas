#pragma once
#include "Defines.h"

#include "ISocketTCP.h"

#include "ISocketUDP.h"

#include "IPAddress.h"

#include "Time/API/Timestamp.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	class LAMBDA_API NetworkUtils
	{
		friend class EngineLoop;

	public:
		DECL_ABSTRACT_CLASS(NetworkUtils);

	public:
		/*
		* Creates a SocketTCP.
		*
		* return - a SocketTCP.
		*/
		static ISocketTCP* CreateSocketTCP();

		/*
		* Creates a SocketUDP.
		*
		* return - a SocketUDP.
		*/
		static ISocketUDP* CreateSocketUDP();

		/*
		* Finds the local network address. Usally 192.168.0.X
		*
		* return - The inet address
		*/
		static IPAddress* GetLocalAddress();

	protected:
		static IPAddress* CreateIPAddress(const std::string& address, uint64 hash);

		static bool Init();
		static void Tick(Timestamp dt);
		static void FixedTick(Timestamp dt);
		static void Release();
	};
}
