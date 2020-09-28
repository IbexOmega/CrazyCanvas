#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Containers/CCBuffer.h"
#include "Containers/TArray.h"

namespace LambdaEngine
{
	struct GameState
	{
		int32 SimulationTick = -1;
		glm::vec3 Position;
		int8 DeltaForward = 0;
		int8 DeltaLeft = 0;
	};

	class ClientSystem : public System, protected IClientHandler
	{
		friend class EngineLoop;

	public:
		DECL_UNIQUE_CLASS(ClientSystem);
		virtual ~ClientSystem();

		bool Connect(IPAddress* address);

		void Tick(Timestamp deltaTime) override;

		void FixedTickMainThread(Timestamp deltaTime);
		void TickMainThread(Timestamp deltaTime);

	protected:
		virtual void OnConnecting(IClient* pClient) override;
		virtual void OnConnected(IClient* pClient) override;
		virtual void OnDisconnecting(IClient* pClient) override;
		virtual void OnDisconnected(IClient* pClient) override;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) override;
		virtual void OnClientReleased(IClient* pClient) override;
		virtual void OnServerFull(IClient* pClient) override;

		void CreateEntity(int32 networkUID, const glm::vec3& position, const glm::vec3& color);

	public:
		static ClientSystem& GetInstance()
		{
			if (!s_pInstance)
				s_pInstance = DBG_NEW ClientSystem();
			return *s_pInstance;
		}

	private:
		ClientSystem();
		void Reconcile();
		void PlayerUpdate(const GameState& gameState);

	private:
		static void StaticFixedTickMainThread(Timestamp deltaTime);
		static void StaticTickMainThread(Timestamp deltaTime);
		static void StaticRelease();

	private:
		IDVector m_ControllableEntities;
		int32 m_NetworkUID;
		ClientBase* m_pClient;
		TArray<GameState> m_FramesToReconcile;
		TArray<GameState> m_FramesProcessedByServer;
		int32 m_SimulationTick;
		int32 m_LastNetworkSimulationTick;
		std::unordered_map<int32, Entity> m_Entities; // <Network, Client>

	private:
		static ClientSystem* s_pInstance;
	};
}