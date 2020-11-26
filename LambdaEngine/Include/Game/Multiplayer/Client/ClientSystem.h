#pragma once

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/UDP/INetworkDiscoveryClient.h"

#include "Application/API/Events/NetworkEvents.h"

namespace LambdaEngine
{
	struct ClientSystemDesc : public ClientDesc
	{
		String Name;
	};

	class ClientSystem : protected IClientHandler, protected INetworkDiscoveryClient
	{
		friend class EngineLoop;
		friend class SingleplayerInitializer;

	public:
		DECL_UNIQUE_CLASS(ClientSystem);
		virtual ~ClientSystem();

		bool Connect(const IPEndPoint& endPoint);

		ClientBase* GetClient();

	protected:
		void TickMainThread(Timestamp deltaTime);

		virtual void OnConnecting(IClient* pClient) override;
		virtual void OnConnected(IClient* pClient) override;
		virtual void OnDisconnecting(IClient* pClient, const String& reason) override;
		virtual void OnDisconnected(IClient* pClient, const String& reason) override;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) override;
		virtual void OnClientReleased(IClient* pClient) override;
		virtual void OnServerFull(IClient* pClient) override;
		virtual void OnServerNotAccepting(IClient* pClient) override;

		virtual void OnServerFound(BinaryDecoder& decoder, const IPEndPoint& endPoint, uint64 serverUID, Timestamp ping, bool isLAN) override;

		bool OnDisconnectedEvent(const ClientDisconnectedEvent& event);

	private:
		ClientSystem(ClientSystemDesc& desc);

	public:
		static ClientSystem& GetInstance()
		{
			return *s_pInstance;
		}

		static void Init(ClientSystemDesc& desc)
		{
			if (!s_pInstance)
				s_pInstance = DBG_NEW ClientSystem(desc);
		}

	private:
		static void StaticTickMainThread(Timestamp deltaTime);
		static void StaticRelease();

	private:
		ClientBase* m_pClient;
		String m_Name;
		bool m_DebuggingWindow;

	private:
		static ClientSystem* s_pInstance;
	};
}
