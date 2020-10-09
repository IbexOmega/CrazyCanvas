#pragma once

#include "Game/ECS/Systems/Networking/ClientBaseSystem.h"
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
	class ClientSystem : public ClientBaseSystem, protected IClientHandler
	{
		friend class EngineLoop;

	public:
		DECL_UNIQUE_CLASS(ClientSystem);
		virtual ~ClientSystem();

		bool Connect(IPAddress* pAddress);

	protected:
		virtual void TickMainThread(Timestamp deltaTime) override;
		virtual void FixedTickMainThread(Timestamp deltaTime) override;

		virtual void OnConnecting(IClient* pClient) override;
		virtual void OnConnected(IClient* pClient) override;
		virtual void OnDisconnecting(IClient* pClient) override;
		virtual void OnDisconnected(IClient* pClient) override;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) override;
		virtual void OnClientReleased(IClient* pClient) override;
		virtual void OnServerFull(IClient* pClient) override;

		void CreateEntity(int32 networkUID, const glm::vec3& position, const glm::vec3& color);

	private:
		ClientSystem();

		void Init();

		void OnPacketCreateEntity(IClient* pClient, NetworkSegment* pPacket);

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
