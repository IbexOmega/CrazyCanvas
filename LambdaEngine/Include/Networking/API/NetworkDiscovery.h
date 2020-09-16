#pragma once
#include "Networking/API/IServerHandler.h"
#include "Networking/API/IClientRemoteHandler.h"

#include "Threading/API/SpinLock.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	class ServerUDP;

	class NetworkDiscovery : protected IServerHandler, protected IClientRemoteHandler
	{
	public:
		DECL_UNIQUE_CLASS(NetworkDiscovery);

		static bool EnableDiscovery();

	protected:
		virtual IClientRemoteHandler* CreateClientHandler() override;

		virtual void OnConnecting(IClient* pClient) override;
		virtual void OnConnected(IClient* pClient) override;
		virtual void OnDisconnecting(IClient* pClient) override;
		virtual void OnDisconnected(IClient* pClient) override;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) override;
		virtual void OnClientReleased(IClient* pClient) override;

	private:
		NetworkDiscovery();

		void FixedTick(Timestamp delta);

	private:
		ServerUDP* m_pServer;
		SpinLock m_Lock;

	private:
		static NetworkDiscovery s_Instance;
	};
}
