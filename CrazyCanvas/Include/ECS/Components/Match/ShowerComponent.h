#pragma once

#include "ECS/Component.h"
#include "Time/API/Timestamp.h"

struct ShowerComponent
{
	DECL_COMPONENT(ShowerComponent);
	LambdaEngine::Timestamp ShowerAvailableTimestamp;
	LambdaEngine::Timestamp ShowerCooldown;
};
