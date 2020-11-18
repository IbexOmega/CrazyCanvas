#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"

class PlayerSoundHelper
{
public:
	static void HandleMovementSound(
		const LambdaEngine::PositionComponent& positionComponent,
		const LambdaEngine::VelocityComponent& velocityComponent,
		LambdaEngine::AudibleComponent& audibleComponent,
		const glm::i8vec3& deltaAction,
		bool walking,
		bool inAir);
};