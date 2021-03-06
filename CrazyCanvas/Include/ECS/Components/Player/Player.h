#pragma once
#include "ECS/Component.h"

#include "Game/ECS/Components/Team/TeamComponent.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

namespace LambdaEngine
{
	class PlayerGroup : public IComponentGroup
	{
	public:
		TArray<ComponentAccess> ToArray() const override final
		{
			return
			{
				PlayerBaseComponent,
				Position,
				Scale,
				Rotation,
				Velocity,
				CharacterCollider,
				Health
			};
		}

	public:
		GroupedComponent<PlayerBaseComponent> PlayerBaseComponent;

		// Transform
		GroupedComponent<PositionComponent>	Position;
		GroupedComponent<ScaleComponent>	Scale;
		GroupedComponent<RotationComponent>	Rotation;

		GroupedComponent<VelocityComponent>	Velocity;

		GroupedComponent<CharacterColliderComponent> CharacterCollider;

		GroupedComponent<HealthComponent> Health;
	};
}