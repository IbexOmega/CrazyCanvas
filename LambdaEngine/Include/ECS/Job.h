#pragma once

#include "Containers/TArray.h"
#include "ECS/EntitySubscriber.h"

namespace LambdaEngine
{
	#define PHASE_COUNT 4u
	#define LAST_PHASE PHASE_COUNT - 1u

	struct Job
	{
		TArray<ComponentAccess> Components;
		std::function<void()> Function;
	};

	struct RegularJob : Job
	{
		float32 TickPeriod;
		float32 Accumulator;
	};
}
