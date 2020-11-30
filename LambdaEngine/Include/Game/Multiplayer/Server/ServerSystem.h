#pragma once

#include "ECS/System.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/UDP/INetworkDiscoveryServer.h"
#include "Networking/API/UDP/ClientRemoteUDP.h"

namespace LambdaEngine
{
	struct ServerSystemDesc : public ServerDesc
	{
		String Name;
	};

	class ServerSystem : public IServerHandler, public INetworkDiscoveryServer
	{
		friend class EngineLoop;

	public:
		DECL_UNIQUE_CLASS(ServerSystem);
		virtual ~ServerSystem();

		bool Start();
		void Stop();

		void TickMainThread(Timestamp deltaTime);

		ServerBase* GetServer();

	protected:
		virtual IClientRemoteHandler* CreateClientHandler() override;

		virtual void OnNetworkDiscoveryPreTransmit(BinaryEncoder& encoder) override;

	public:
		static ServerSystem& GetInstance()
		{
			return *s_pInstance;
		}

		static void Init(ServerSystemDesc& desc)
		{
			if (!s_pInstance)
				s_pInstance = DBG_NEW ServerSystem(desc);
		}

	private:
		ServerSystem(ServerSystemDesc& desc);

	private:
		static void StaticTickMainThread(Timestamp deltaTime);
		static void StaticRelease();

	private:
		IDVector					m_NetworkEntities;
		ServerBase*					m_pServer;
		String						m_Name;

	private:
		static ServerSystem* s_pInstance;
	};
}