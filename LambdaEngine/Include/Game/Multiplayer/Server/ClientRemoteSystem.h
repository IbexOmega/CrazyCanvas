#pragma once

#include "Networking/API/PlatformNetworkUtils.h"

namespace LambdaEngine
{
	class ClientRemoteSystem : public IClientRemoteHandler
	{
		friend class ServerSystem;

	public:
		DECL_UNIQUE_CLASS(ClientRemoteSystem);
		virtual ~ClientRemoteSystem();

	protected:
		virtual void OnConnecting(IClient* pClient) override;
		virtual void OnConnected(IClient* pClient) override;
		virtual void OnDisconnecting(IClient* pClient, const String& reason) override;
		virtual void OnDisconnected(IClient* pClient, const String& reason) override;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) override;
		virtual void OnClientReleased(IClient* pClient) override;

	private:
		ClientRemoteSystem();

	private:
		ClientRemoteBase* m_pClient;
	};
}