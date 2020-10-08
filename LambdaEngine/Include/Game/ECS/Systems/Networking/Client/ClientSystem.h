#pragma once

#include "Game/ECS/Systems/Networking/ClientBaseSystem.h"
#include "Game/ECS/Systems/Physics/CharacterControllerSystem.h"
#include "Game/ECS/Systems/Networking/Client/NetworkPositionSystem.h"
#include "Game/ECS/Systems/Actions/PlayerActionSystem.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Containers/CCBuffer.h"
#include "Containers/TArray.h"

namespace LambdaEngine
{
	typedef std::unordered_map<uint16, TArray<std::function<void(NetworkSegment*)>>> PacketSubscriberMap;

	class InterpolationSystem;

	class ClientSystem : public ClientBaseSystem, protected IClientHandler
	{
		friend class EngineLoop;

	public:
		DECL_UNIQUE_CLASS(ClientSystem);
		virtual ~ClientSystem();

		bool Connect(IPAddress* pAddress);

		void SubscribeToPacketType(uint16 packetType, const std::function<void(NetworkSegment*)>& func);
		Entity GetEntityFromNetworkUID(int32 networkUID) const;
		bool IsLocalClient(int32 networkUID) const;

	protected:
		virtual void TickMainThread(Timestamp deltaTime) override;
		virtual void FixedTickMainThread(Timestamp deltaTime) override;
		virtual Entity GetEntityPlayer() const override;

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
		void Reconcile();
		void SendGameState(const GameState& gameState);

		void ReplayGameStatesBasedOnServerGameState(GameState* pGameStates, uint32 count, const GameState& gameStateServer);
		bool CompareGameStates(const GameState& gameStateLocal, const GameState& gameStateServer);

		void OnPacketPlayerAction(NetworkSegment* pPacket);
		void OnPacketCreateEntity(NetworkSegment* pPacket);

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
		IDVector m_ControllableEntities;
		int32 m_NetworkUID;
		ClientBase* m_pClient;
		TArray<GameState> m_FramesToReconcile;
		TArray<GameState> m_FramesProcessedByServer;
		int32 m_SimulationTick;
		int32 m_LastNetworkSimulationTick;
		std::unordered_map<int32, Entity> m_Entities; // <NetworkUID, Entity>
		PacketSubscriberMap m_PacketSubscribers;

		CharacterControllerSystem m_CharacterControllerSystem;
		NetworkPositionSystem m_NetworkPositionSystem;
		PlayerActionSystem m_PlayerActionSystem;

	private:
		static ClientSystem* s_pInstance;
	};
}
