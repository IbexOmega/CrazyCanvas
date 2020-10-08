#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "Networking/API/PlatformNetworkUtils.h"

namespace LambdaEngine
{
	class ServerSystem : public System, public IServerHandler
	{
		friend class EngineLoop;

	public:
		DECL_UNIQUE_CLASS(ServerSystem);
		virtual ~ServerSystem();

		bool Start();
		void Stop();

		void Tick(Timestamp deltaTime) override;

		void FixedTickMainThread(Timestamp deltaTime);
		void TickMainThread(Timestamp deltaTime);

		int32 GetSimulationTick() const;

	protected:
		virtual IClientRemoteHandler* CreateClientHandler() override;

	public:
		static ServerSystem& GetInstance()
		{
			if (!s_pInstance)
				s_pInstance = DBG_NEW ServerSystem();
			return *s_pInstance;
		}

	private:
		ServerSystem();

	private:
		static void StaticFixedTickMainThread(Timestamp deltaTime);
		static void StaticTickMainThread(Timestamp deltaTime);
		static void StaticRelease();

	private:
		IDVector	m_NetworkEntities;
		ServerBase* m_pServer;
		int32		m_SimulationTick;

	private:
		static ServerSystem* s_pInstance;
	};
}