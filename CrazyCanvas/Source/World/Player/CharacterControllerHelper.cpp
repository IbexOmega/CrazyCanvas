#include "World/Player/CharacterControllerHelper.h"
#include "World/Player/PlayerSettings.h"

#include "ECS/ECSCore.h"

#include "Physics/PhysX/PhysX.h"

using namespace physx;
using namespace LambdaEngine;

void CharacterControllerHelper::SetForeignCharacterController(
	LambdaEngine::CharacterColliderComponent& characterColliderComponent,
	const LambdaEngine::NetworkPositionComponent& networkPositionComponent)
{
	PxController* pController = characterColliderComponent.pController;
	const glm::vec3& position = networkPositionComponent.Position;
	pController->setFootPosition({ position.x,  position.y,  position.z });
}

void CharacterControllerHelper::TickForeignCharacterController(
	float32 dt,
	LambdaEngine::CharacterColliderComponent& characterColliderComponent,
	const LambdaEngine::NetworkPositionComponent& networkPositionComponent,
	LambdaEngine::VelocityComponent& velocityComponent)
{
	PxController* pController = characterColliderComponent.pController;

	const PxExtendedVec3 oldPositionPX = pController->getFootPosition();
	PxVec3 translationPX =
	{
		networkPositionComponent.Position.x - (float)oldPositionPX.x,
		networkPositionComponent.Position.y - (float)oldPositionPX.y,
		networkPositionComponent.Position.z - (float)oldPositionPX.z
	};

	pController->move(translationPX, 0.0f, dt, characterColliderComponent.Filters);

	if (glm::length2(velocityComponent.Velocity) > glm::epsilon<float>())
	{
		// Disable vertical movement if the character is on the ground
		PxControllerState controllerState;
		pController->getState(controllerState);
		if (controllerState.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN)
		{
			velocityComponent.Velocity.y = 0.0f;
		}
	}
}

/*
* Sets the position of the PxController taken from the PositionComponent.
* Move the PxController using the VelocityComponent by the deltatime.
* Calculates a new Velocity based on the difference of the last position and the new one.
* Sets the new position of the PositionComponent
*/
void CharacterControllerHelper::TickCharacterController(
	float32 dt,
	LambdaEngine::CharacterColliderComponent& characterColliderComponent,
	const LambdaEngine::NetworkPositionComponent& networkPositionComponent,
	LambdaEngine::VelocityComponent& velocityComponent)
{
	PxController* pController = characterColliderComponent.pController;

	// Update velocity
	glm::vec3& velocity = velocityComponent.Velocity;
	ApplyGravity(dt, velocity);

	// Calculate Tick Translation
	PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
	translationPX *= dt;

	// Distance between the capsule's feet to its center position. Includes contact offset.
	const glm::vec3& position = networkPositionComponent.Position;
	if (networkPositionComponent.Dirty)
	{
		pController->setFootPosition({ position.x,  position.y,  position.z });
	}

	pController->move(translationPX, 0.0f, dt, characterColliderComponent.Filters);

	const PxExtendedVec3& newPositionPX = pController->getFootPosition();
	velocity =
	{
		(float)newPositionPX.x - position.x,
		(float)newPositionPX.y - position.y,
		(float)newPositionPX.z - position.z
	};
	velocity /= dt;

	if (glm::length2(velocity) > glm::epsilon<float>())
	{
		// Disable vertical movement if the character is on the ground
		PxControllerState controllerState;
		pController->getState(controllerState);
		if (controllerState.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN)
		{
			velocity.y = 0.0f;
		}

		// Update entity's position
		NetworkPositionComponent& positionCompMutable = const_cast<NetworkPositionComponent&>(networkPositionComponent);
		positionCompMutable.Position	= { newPositionPX.x, newPositionPX.y, newPositionPX.z };
		positionCompMutable.Dirty		= true;
	}
}

void CharacterControllerHelper::ApplyGravity(float32 dt, glm::vec3& velocity)
{
	float32 gravityMultiplier = 1.0f + float32(velocity.y < 0.0f) * PLAYER_FALLING_MULTIPLIER;
	velocity.y -= GRAVITATIONAL_ACCELERATION * gravityMultiplier * dt;
}
