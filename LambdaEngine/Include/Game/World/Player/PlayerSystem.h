#pragma once
#include "ECS/System.h"

#include "Game/World/Player/PlayerActionSystem.h"

namespace LambdaEngine
{
	struct GameState;
	class NetworkSegment;
	class IClient;

	struct GameStateOther
	{
		glm::vec3 Position;
		glm::vec3 Velocity;
		bool HasNewData = false;
	};

	class PlayerSystem
	{
		friend class ClientSystem;

	public:
		DECL_UNIQUE_CLASS(PlayerSystem);
		virtual ~PlayerSystem();

		void Init();

		void FixedTickMainThread(Timestamp deltaTime, IClient* pClient);

		void TickLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState);
		void TickOtherPlayersAction(Timestamp deltaTime);

	private:
		PlayerSystem();

		void SendGameState(const GameState& gameState, IClient* pClient);
		void OnPacketPlayerAction(IClient* pClient, NetworkSegment* pPacket);
		void Reconcile();
		void ReplayGameStatesBasedOnServerGameState(GameState* pGameStates, uint32 count, const GameState& gameStateServer);
		bool CompareGameStates(const GameState& gameStateLocal, const GameState& gameStateServer);

	private:
		std::unordered_map<Entity, GameStateOther> m_EntityOtherStates;
		PlayerActionSystem m_PlayerActionSystem;

		int32 m_NetworkUID;
		int32 m_SimulationTick;
		TArray<GameState> m_FramesToReconcile;
		TArray<GameState> m_FramesProcessedByServer;
	};
}