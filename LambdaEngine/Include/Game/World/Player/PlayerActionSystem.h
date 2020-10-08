#pragma once
#include "ECS/System.h"

namespace LambdaEngine
{
	struct GameState;

	struct GameStateOther
	{
		glm::vec3 Position;
		glm::vec3 Velocity;
		bool HasNewData = false;
	};

	class PlayerActionSystem
	{
		friend class ClientSystem;

	public:
		DECL_UNIQUE_CLASS(PlayerActionSystem);
		virtual ~PlayerActionSystem();

		void Init();

		void TickLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState);
		void TickOtherPlayersAction(Timestamp deltaTime);

		void OnPacketPlayerAction(Entity entity, const GameState* pGameState);

	private:
		PlayerActionSystem();

		void DoLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState);

	public:
		static void ComputeVelocity(int8 deltaForward, int8 deltaLeft, glm::vec3& result);

	private:

		std::unordered_map<Entity, GameStateOther> m_EntityOtherStates;
	};
}