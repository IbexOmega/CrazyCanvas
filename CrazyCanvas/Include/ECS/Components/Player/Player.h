#pragma once

#include "ECS/Component.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"

namespace LambdaEngine
{
	struct PlayerTag
	{
		DECL_COMPONENT(PlayerTag);
	};

	class PlayerGroup : public IComponentGroup
	{
	public:
		TArray<ComponentAccess> ToArray() const override final
		{
			return
			{
				PlayerTag, Position, Scale, Rotation, Velocity, CharacterCollider
			};
		}

	public:
		GroupedComponent<PlayerTag> PlayerTag;

		// Transform
		GroupedComponent<PositionComponent>	Position;
		GroupedComponent<ScaleComponent>	Scale;
		GroupedComponent<RotationComponent>	Rotation;

		GroupedComponent<VelocityComponent>	Velocity;

		GroupedComponent<CharacterColliderComponent> CharacterCollider;
	};
}
