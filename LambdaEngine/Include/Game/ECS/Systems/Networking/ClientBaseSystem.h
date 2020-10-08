#pragma once

#include "ECS/System.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct GameState
	{
		int32 SimulationTick = -1;
		glm::vec3 Position;
		int8 DeltaForward = 0;
		int8 DeltaLeft = 0;
	};

	struct GameStateComparator
	{
		bool operator() (const GameState& lhs, const GameState& rhs) const
		{
			return lhs.SimulationTick < rhs.SimulationTick;
		}
	};

	class ClientBaseSystem : public System
	{
		friend class ServerSystem;

	public:
		DECL_UNIQUE_CLASS(ClientBaseSystem);
		virtual ~ClientBaseSystem();

		void Tick(Timestamp deltaTime) override;

	protected:
		ClientBaseSystem();

		virtual void TickMainThread(Timestamp deltaTime) = 0;
		virtual void FixedTickMainThread(Timestamp deltaTime) = 0;
		virtual Entity GetEntityPlayer() const = 0;

		void PlayerUpdate(Entity entity, const GameState& gameState);

	private:
		
	};
}