#pragma once

#include "Threading/API/SpinLock.h"

#include "Time/API/Timestamp.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	class INetworkDiscoveryServer;
	class INetworkDiscoveryClient;
	class ServerNetworkDiscovery;
	class ClientNetworkDiscovery;

	class NetworkDiscovery
	{
		friend class NetworkUtils;

	public:
		DECL_STATIC_CLASS(NetworkDiscovery);

		static bool EnableServer(const String& nameOfGame, uint16 portOfGameServer, INetworkDiscoveryServer* pHandler, uint16 portOfBroadcastServer = 4450);
		static void DisableServer();
		static bool IsServerEnabled();

		static bool EnableClient(const String& nameOfGame, INetworkDiscoveryClient* pHandler, uint16 portOfBroadcastServer = 4450, Timestamp searchInterval = Timestamp::Seconds(1));
		static void DisableClient();
		static bool IsClientEnabled();

	private:
		static void FixedTickStatic(Timestamp delta);
		static void ReleaseStatic();

	private:
		static ServerNetworkDiscovery* s_pServer;
		static ClientNetworkDiscovery* s_pClient;
		static SpinLock s_Lock;
	};
}
