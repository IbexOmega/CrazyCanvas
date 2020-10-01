#pragma once

#include "Containers/TArray.h"
#include "ECS/EntitySubscriber.h"

namespace LambdaEngine
{
	const uint32 g_PhaseCount = 3;
	const uint32 g_LastPhase = g_PhaseCount - 1;

	struct Job
	{
		std::function<void()> Function;
		TArray<ComponentAccess> Components;
	};
}
