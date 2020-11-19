#pragma once

#include "Types.h"
#include "Math/Math.h"

struct PlayerGameState
{
	int32 SimulationTick = -1;
	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::quat Rotation;
	glm::i8vec3 DeltaAction;
	bool Walking;
};

struct GameStateComparator
{
	bool operator() (const PlayerGameState& lhs, const PlayerGameState& rhs) const
	{
		return lhs.SimulationTick < rhs.SimulationTick;
	}
};