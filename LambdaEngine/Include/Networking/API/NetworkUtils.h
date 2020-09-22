#pragma once
#include "Defines.h"

#include "Networking/API/TCP/ISocketTCP.h"
#include "Networking/API/UDP/ISocketUDP.h"
#include "Networking/API/TCP/ClientTCP.h"
#include "Networking/API/UDP/ClientUDP.h"
#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/TCP/ServerTCP.h"
#include "Networking/API/UDP/ServerUDP.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/UDP/NetworkDiscovery.h"

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

		static ClientBase* CreateClient(const ClientDesc& desc);
		static ServerBase* CreateServer(const ServerDesc& desc);

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
		static void PreRelease();
		static void PostRelease();
	};
}
