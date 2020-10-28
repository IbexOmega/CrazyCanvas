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
		
		String targetStateName = pCurrentTransition != nullptr ? pCurrentTransition->GetToState()->GetName() : "";
		String currentStateName = pCurrentAnimationState->GetName();

		bool idling = glm::length2(velocityComponent.Velocity) < 0.5f;

		if (idling)
		{
			if (currentStateName != "Idle" && targetStateName != "Idle")
			{
				animationComponent.pGraph->TransitionToState("Idle");
			}
		}
#ifndef LAMBDA_DEBUG
		else 
		{
			glm::vec3 forwardDirection = GetForward(rotationComponent.Quaternion);
			glm::vec3 rightDirection = GetRight(rotationComponent.Quaternion);
			float32 dotFV = glm::dot(forwardDirection, velocityComponent.Velocity);
			float32 dotRV = glm::dot(rightDirection, velocityComponent.Velocity);
			float32 epsilon = 0.01f;
			bool runningForward		= dotFV > epsilon;
			bool runningBackward	= dotFV < -epsilon;
			bool strafingRight		= dotRV > epsilon;
			bool strafingLeft		= dotRV < -epsilon;

			if (runningForward)
			{
				if (strafingRight)
				{
					if (currentStateName != "Running & Strafe Right" && targetStateName != "Running & Strafe Right")
					{
						animationComponent.pGraph->TransitionToState("Running & Strafe Right");
					}
				}
				else if (strafingLeft)
				{
					if (currentStateName != "Running & Strafe Left" && targetStateName != "Running & Strafe Left")
					{
						animationComponent.pGraph->TransitionToState("Running & Strafe Left");
					}
				}
				else
				{
					if (currentStateName != "Running" && targetStateName != "Running")
					{
						animationComponent.pGraph->TransitionToState("Running");
					}
				}
			}
			else if (runningBackward)
			{
				if (strafingRight)
				{
					if (currentStateName != "Run Backward & Strafe Right" && targetStateName != "Run Backward & Strafe Right")
					{
						animationComponent.pGraph->TransitionToState("Run Backward & Strafe Right");
					}
				}
				else if (strafingLeft)
				{
					if (currentStateName != "Run Backward & Strafe Left" && targetStateName != "Run Backward & Strafe Left")
					{
						animationComponent.pGraph->TransitionToState("Run Backward & Strafe Left");
					}
				}
				else
				{
					if (currentStateName != "Run Backward" && targetStateName != "Run Backward")
					{
						animationComponent.pGraph->TransitionToState("Run Backward");
					}
				}
			}
			else if (strafingRight)
			{
				if (currentStateName != "Strafe Right" && targetStateName != "Strafe Right")
				{
					animationComponent.pGraph->TransitionToState("Strafe Right");
				}
			}
			else if (strafingLeft)
			{
				if (currentStateName != "Strafe Left" && targetStateName != "Strafe Left")
				{
					animationComponent.pGraph->TransitionToState("Strafe Left");
				}
			}
		}
#endif
	}
}
