#include "ECS/Systems/Player/PlayerAnimationSystem.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/ECSCore.h"
#include "ECS/Systems/Player/WeaponSystem.h"

#include "Game/ECS/Components/Rendering/AnimationComponent.h"

PlayerAnimationSystem::PlayerAnimationSystem()
{
}

PlayerAnimationSystem::~PlayerAnimationSystem()
{
}

bool PlayerAnimationSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		PlayerGroup playerGroup;
		playerGroup.Velocity.Permissions = R;
		playerGroup.Rotation.Permissions = R;

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_PlayerEntities,
				.ComponentAccesses =
				{
					{RW, AnimationComponent::Type()}
				},
				.ComponentGroups = { &playerGroup }
			}
		};
		systemReg.Phase = 1u;

		RegisterSystem(TYPE_NAME(PlayerAnimationSystem), systemReg);
	}

	return true;
}

void PlayerAnimationSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<AnimationComponent>* pAnimationComponents = pECS->GetComponentArray<AnimationComponent>();
	const ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
	const ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();

	for (Entity playerEntity : m_PlayerEntities)
	{
		AnimationComponent& animationComponent = pAnimationComponents->GetData(playerEntity);
		const VelocityComponent& velocityComponent = pVelocityComponents->GetConstData(playerEntity);
		const RotationComponent& rotationComponent = pRotationComponents->GetConstData(playerEntity);

		Transition* pCurrentTransition = animationComponent.pGraph->GetCurrentTransition();
		AnimationState* pCurrentAnimationState = animationComponent.pGraph->GetCurrentState();
		
		String currentStateName;

		if (pCurrentTransition != nullptr)
		{
			currentStateName = pCurrentTransition->GetToState()->GetName();
		}
		else
		{
			currentStateName = pCurrentAnimationState->GetName();
		}

		if (glm::length2(velocityComponent.Velocity) < 0.5f && currentStateName != "Idle")
		{
			animationComponent.pGraph->TransitionToState("Idle");
		}
		else
		{
			animationComponent.pGraph->TransitionToState("Running");
		}
	}
}
