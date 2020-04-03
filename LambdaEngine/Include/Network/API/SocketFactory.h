#pragma once

#include "Defines.h"
#include <string>

namespace LambdaEngine
{
	class ISocketTCP;
	class ISocketUDP;

	class LAMBDA_API SocketFactory
	{
	public:
		DECL_ABSTRACT_CLASS(SocketFactory);

	public:
		/*
		* Creates a SocketTCP.
		*
		* return - a SocketTCP.
		*/
		static ISocketTCP* CreateSocketTCP() { return nullptr; };

		/*
		* Creates a SocketUDP.
		*
		* return - a SocketUDP.
		*/
		static ISocketUDP* CreateSocketUDP() { return nullptr; };

		/*
		* Finds the local network address. Usally 192.168.0.X
		*
		* return - The inet address
		*/
		static const std::string& GetLocalAddress() { return ""; };

	private:
		static bool Init() { return false; };
		static void Release() {};
	};
}
