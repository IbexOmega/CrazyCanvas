#include "ECS/Systems/Player/WeaponSystem.h"

#include "Input/API/InputActionSystem.h"
// #include "Game/ECS/Components/"

bool WeaponSystem::Init()
{
	using namespace LambdaEngine;

	{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					.pSubscriber = &m_CameraEntities,
					.ComponentAccesses =
					{
						{R, CameraComponent::Type()}, {NDA, ViewProjectionMatricesComponent::Type()}, {RW, VelocityComponent::Type()},
						{NDA, PositionComponent::Type()}, {RW, RotationComponent::Type()}
					}
				}
			};
			systemReg.SubscriberRegistration.AdditionalAccesses = { {{R, FreeCameraComponent::Type()}, {R, FPSControllerComponent::Type()}} };
			systemReg.Phase = 0;

			RegisterSystem(systemReg);
		}

		return true;
}

void WeaponSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	InputActionSystem::
}
