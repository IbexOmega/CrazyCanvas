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

	static void TickForeignCharacterController(float32 dt,
		LambdaEngine::Entity entity,
		LambdaEngine::ComponentArray<LambdaEngine::CharacterColliderComponent>* pCharacterColliders,
		const LambdaEngine::ComponentArray<LambdaEngine::NetworkPositionComponent>* pNetPosComponents,
		LambdaEngine::ComponentArray<LambdaEngine::VelocityComponent>* pVelocityComponents);

	static void TickCharacterController(float32 dt,
		LambdaEngine::Entity entity,
		LambdaEngine::ComponentArray<LambdaEngine::CharacterColliderComponent>* pCharacterColliders,
		const LambdaEngine::ComponentArray<LambdaEngine::NetworkPositionComponent>* pNetPosComponents,
		LambdaEngine::ComponentArray<LambdaEngine::VelocityComponent>* pVelocityComponents);
};
