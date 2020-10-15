#pragma once

#include "ECS/System.h"

#include "Game/ECS/Systems/Physics/CharacterControllerSystem.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/UDP/INetworkDiscoveryServer.h"

namespace LambdaEngine
{
	class ServerSystem : public IServerHandler, public INetworkDiscoveryServer
	{
		friend class EngineLoop;

	public:
		DECL_UNIQUE_CLASS(ServerSystem);
		virtual ~ServerSystem();

		bool Start();
		void Stop();

		void FixedTickMainThread(Timestamp deltaTime);
		void TickMainThread(Timestamp deltaTime);

		int32 GetSimulationTick() const;

	protected:
		virtual IClientRemoteHandler* CreateClientHandler() override;

		virtual void OnNetworkDiscoveryPreTransmit(BinaryEncoder& encoder) override;

	public:
		static ServerSystem& GetInstance()
		{
			return *s_pInstance;
		}

		static void Init(const String& name)
		{
			if (!s_pInstance)
				s_pInstance = DBG_NEW ServerSystem(name);
		}

	private:
		ServerSystem(const String& name);

	private:
		static void StaticFixedTickMainThread(Timestamp deltaTime);
		static void StaticTickMainThread(Timestamp deltaTime);
		static void StaticRelease();

	private:
		IDVector					m_NetworkEntities;
		ServerBase*					m_pServer;
		CharacterControllerSystem	m_CharacterControllerSystem;
		String						m_Name;

	private:
		static ServerSystem* s_pInstance;
	};
}