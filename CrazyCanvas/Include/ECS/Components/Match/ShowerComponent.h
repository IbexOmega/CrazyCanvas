#pragma once

struct ShowerComponent
{
	DECL_COMPONENT(ShowerComponent);
	LambdaEngine::Timestamp ShowerAvailableTimestamp;
	LambdaEngine::Timestamp ShowerCooldown;
};