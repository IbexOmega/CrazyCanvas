#pragma once
#include "Defines.h"

#include "TCP/ISocketTCP.h"
#include "TCP/IClientTCP.h"
#include "TCP/IClientTCPHandler.h"
#include "TCP/ServerTCP.h"

#include "UDP/ISocketUDP.h"
#include "UDP/IClientUDP.h"
#include "UDP/IClientUDPHandler.h"
#include "UDP/ServerUDP.h"

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
		static ISocketTCP* CreateSocketTCP() { return nullptr; };

		/*
		* Creates a SocketUDP.
		*
		* return - a SocketUDP.
		*/
		static ISocketUDP* CreateSocketUDP() { return nullptr; };

		/*
		* Creates a IClientTCP.
		*
		* return - a IClientTCP.
		*/
		static IClientTCP* CreateClientTCP(IClientTCPHandler* handler);

		/*
		* Creates a IClientUDP.
		*
		* return - a IClientUDP.
		*/
		static IClientUDP* CreateClientUDP(IClientUDPHandler* handler);

		/*
		* Creates a ServerTCP.
		*
		* return - a ServerTCP.
		*/
		static ServerTCP* CreateServerTCP(IServerTCPHandler* handler, uint16 maxClients);

		/*
		* Creates a ServerUDP.
		*
		* return - a ServerUDP.
		*/
		static ServerUDP* CreateServerUDP(IServerUDPHandler* handler);

		/*
		* Finds the local network address. Usally 192.168.0.X
		*
		* return - The inet address
		*/
		static std::string GetLocalAddress();

	protected:
		static bool Init();
		static void Tick(Timestamp dt);
		static void Release();
	};
}
