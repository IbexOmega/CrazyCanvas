#pragma once
#include "Networking/API/IServerHandler.h"
#include "Networking/API/IClientHandler.h"
#include "Networking/API/UDP/ServerNetworkDiscovery.h"

#include "Threading/API/SpinLock.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	class ServerUDP;
	class ClientUDP;
	class INetworkDiscoveryServer;
	class INetworkDiscoveryClient;

	class NetworkDiscovery : protected IServerHandler, protected IClientHandler
	{
		friend class NetworkUtils;

	public:
		DECL_UNIQUE_CLASS(NetworkDiscovery);

		static bool EnableServer(uint16 portOfGameServer, INetworkDiscoveryServer* pHandler, uint16 portOfBroadcastServer = 4450);
		static void DisableServer();
		static bool IsServerEnabled();

		static bool EnableClient(INetworkDiscoveryClient* pHandler);
		static void DisableClient();
		static bool IsClientEnabled();

	protected:
		virtual IClientRemoteHandler* CreateClientHandler() override;

		virtual void OnConnecting(IClient* pClient) override;
		virtual void OnConnected(IClient* pClient) override;
		virtual void OnDisconnecting(IClient* pClient) override;
		virtual void OnDisconnected(IClient* pClient) override;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) override;
		virtual void OnClientReleased(IClient* pClient) override;
		virtual void OnServerFull(IClient* pClient) override;

	private:
		NetworkDiscovery();

	private:
		static void FixedTick(Timestamp delta);
		static void ReleaseStatic();

	private:
		ServerNetworkDiscovery* m_pServer;
		ClientUDP* m_pClient;
		SpinLock m_Lock;

	private:
		static NetworkDiscovery s_Instance;
	};
}
