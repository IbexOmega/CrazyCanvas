#include "World/Player/CharacterControllerHelper.h"

#include "ECS/ECSCore.h"

#include "Physics/PhysX/PhysX.h"

using namespace physx;
using namespace LambdaEngine;

void CharacterControllerHelper::TickForeignCharacterController(
	float32 dt,
	Entity entity,
	ComponentArray<CharacterColliderComponent>* pCharacterColliders,
	const ComponentArray<NetworkPositionComponent>* pNetPosComponents,
	ComponentArray<VelocityComponent>* pVelocityComponents)
{
	CharacterColliderComponent& characterCollider	= pCharacterColliders->GetData(entity);
	const NetworkPositionComponent& positionComp	= pNetPosComponents->GetConstData(entity);
	VelocityComponent& velocityComp					= pVelocityComponents->GetData(entity);

	PxController* pController = characterCollider.pController;

	const PxExtendedVec3 oldPositionPX = pController->getFootPosition();
	PxVec3 translationPX =
	{
		positionComp.Position.x - (float)oldPositionPX.x,
		positionComp.Position.y - (float)oldPositionPX.y,
		positionComp.Position.z - (float)oldPositionPX.z
	};

	pController->move(translationPX, 0.0f, dt, characterCollider.Filters);

	if (glm::length2(velocityComp.Velocity) > glm::epsilon<float>())
	{
		// Disable vertical movement if the character is on the ground
		PxControllerState controllerState;
		pController->getState(controllerState);
		if (controllerState.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN)
		{
			velocityComp.Velocity.y = 0.0f;
		}
	}
	// Maybe add something to change the rendered PositionComponent here in case we collide
}

/*
* Sets the position of the PxController taken from the PositionComponent.
* Move the PxController using the VelocityComponent by the deltatime.
* Calculates a new Velocity based on the difference of the last position and the new one.
* Sets the new position of the PositionComponent
*/
void CharacterControllerHelper::TickCharacterController(
	float32 dt,
	Entity entity,
	ComponentArray<CharacterColliderComponent>* pCharacterColliders,
	const ComponentArray<NetworkPositionComponent>* pNetPosComponents,
	ComponentArray<VelocityComponent>* pVelocityComponents)
{
	CharacterColliderComponent& characterCollider	= pCharacterColliders->GetData(entity);
	const NetworkPositionComponent& positionComp	= pNetPosComponents->GetConstData(entity);
	VelocityComponent& velocityComp					= pVelocityComponents->GetData(entity);

	PxController* pController = characterCollider.pController;

	// Update velocity
	glm::vec3& velocity = velocityComp.Velocity;
	velocity.y -= GRAVITATIONAL_ACCELERATION * dt;

	// Calculate Tick Translation
	PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
	translationPX *= dt;

	// Distance between the capsule's feet to its center position. Includes contact offset.
	const glm::vec3& position = positionComp.Position;
	if (positionComp.Dirty)
	{
		pController->setFootPosition({ position.x,  position.y,  position.z });
	}

	pController->move(translationPX, 0.0f, dt, characterCollider.Filters);

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
		NetworkPositionComponent& positionCompMutable = const_cast<NetworkPositionComponent&>(positionComp);
		positionCompMutable.Position	= { newPositionPX.x, newPositionPX.y, newPositionPX.z };
		positionCompMutable.Dirty		= true;
	}
}