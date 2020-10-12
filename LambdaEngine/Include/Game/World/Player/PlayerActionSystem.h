#pragma once

#include "ECS/Entity.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	struct GameState;

	class PlayerActionSystem
	{
		friend class PlayerSystem;

	public:
		DECL_UNIQUE_CLASS(PlayerActionSystem);
		virtual ~PlayerActionSystem();

		void Init();

		void TickMainThread(Timestamp deltaTime, Entity entityPlayer);

	private:
		PlayerActionSystem();

		void DoAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState);

	public:
		static void ComputeVelocity(const glm::quat& rotation, int8 deltaForward, int8 deltaLeft, glm::vec3& result);
	};
}