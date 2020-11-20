#pragma once

struct ShowerComponent
{
	DECL_COMPONENT(ShowerComponent);
	LambdaEngine::Timestamp PickupAvailableTimestamp;
	LambdaEngine::Timestamp PickupCooldown;
};
