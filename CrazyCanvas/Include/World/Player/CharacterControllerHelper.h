#pragma once

#include "ECS/ComponentArray.h"

#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

class CharacterControllerHelper
{
public:
	DECL_STATIC_CLASS(CharacterControllerHelper);

public:

	static void TickForeignCharacterController(
		float32 dt,
		LambdaEngine::CharacterColliderComponent& characterColliderComponent,
		const LambdaEngine::NetworkPositionComponent& networkPositionComponent,
		LambdaEngine::VelocityComponent& velocityComponent);

	static void TickCharacterController(
		float32 dt,
		LambdaEngine::CharacterColliderComponent& characterColliderComponent,
		const LambdaEngine::NetworkPositionComponent& networkPositionComponent,
		LambdaEngine::VelocityComponent& velocityComponent);

	static void ApplyGravity(float32 dt, glm::vec3& velocity);
};
