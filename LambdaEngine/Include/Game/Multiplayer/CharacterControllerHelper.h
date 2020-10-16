#pragma once

#include "ECS/ComponentArray.h"

#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

namespace LambdaEngine
{
	using namespace physx;

	class CharacterControllerHelper
	{
		friend class ClientSystem;
		friend class ServerSystem;

	public:
		DECL_STATIC_CLASS(CharacterControllerHelper);

	public:

		static void TickForeignCharacterController(float32 dt,
			Entity entity,
			ComponentArray<CharacterColliderComponent>* pCharacterColliders,
			const ComponentArray<NetworkPositionComponent>* pNetPosComponents,
			ComponentArray<VelocityComponent>* pVelocityComponents);

		static void TickCharacterController(float32 dt,
			Entity entity,
			ComponentArray<CharacterColliderComponent>* pCharacterColliders,
			const ComponentArray<NetworkPositionComponent>* pNetPosComponents,
			ComponentArray<VelocityComponent>* pVelocityComponents);
	};
}
