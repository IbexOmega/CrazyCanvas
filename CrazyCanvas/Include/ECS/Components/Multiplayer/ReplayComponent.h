#pragma once

#include "ECS/Component.h"

struct ReplayComponent
{
	friend class ReplaySystem;

	DECL_COMPONENT(ReplayComponent);

	static void RegisterPredictionError(int32 simulationTick)
	{
		HasPredictionError = true;

		if (simulationTick < SimulationTickOfError)
			SimulationTickOfError = simulationTick;
	}

	static int32 GetSimulationTickCompleted()
	{
		return SimulationTickCompleted;
	}

private:
	static inline bool HasPredictionError = false;
	static inline int32 SimulationTickOfError = -1;
	static inline int32 SimulationTickCompleted = -1;
};