#pragma once

#include "Game/ECS/Systems/Physics/CharacterControllerSystem.h"
#include "Game/ECS/Systems/Networking/Client/NetworkPositionSystem.h"
#include "Game/World/Player/PlayerSystem.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Containers/CCBuffer.h"
#include "Containers/TArray.h"

namespace LambdaEngine
{
	class ClientSystem : protected IClientHandler
	{
		friend class EngineLoop;

	public:
		DECL_UNIQUE_CLASS(ClientSystem);
		virtual ~ClientSystem();

		bool Connect(IPAddress* pAddress);

	protected:
		void TickMainThread(Timestamp deltaTime);
		void FixedTickMainThread(Timestamp deltaTime);

		virtual void OnConnecting(IClient* pClient) override;
		virtual void OnConnected(IClient* pClient) override;
		virtual void OnDisconnecting(IClient* pClient) override;
		virtual void OnDisconnected(IClient* pClient) override;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) override;
		virtual void OnClientReleased(IClient* pClient) override;
		virtual void OnServerFull(IClient* pClient) override;

	private:
		ClientSystem();

		void Init();

	public:
		static ClientSystem& GetInstance()
		{
			if (!s_pInstance)
			{
				s_pInstance = DBG_NEW ClientSystem();
				s_pInstance->Init();
			}
			return *s_pInstance;
		}

		static bool HasInstance()
		{
			return s_pInstance;
		}

	private:
		static void StaticFixedTickMainThread(Timestamp deltaTime);
		static void StaticTickMainThread(Timestamp deltaTime);
		static void StaticRelease();

	private:
		ClientBase* m_pClient;
		CharacterControllerSystem m_CharacterControllerSystem;
		NetworkPositionSystem m_NetworkPositionSystem;
		PlayerSystem m_PlayerSystem;

	private:
		static ClientSystem* s_pInstance;
	};
}
