#include "Game/ECS/Systems/Physics/CharacterControllerSystem.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"

#include <PxPhysicsAPI.h>

namespace LambdaEngine
{
	// TODO: Temporary solution until there's a separate camera entity with an offset
	constexpr const float characterHeight = 1.8f;

	CharacterControllerSystem::CharacterControllerSystem()
	{

	}

	CharacterControllerSystem::~CharacterControllerSystem()
	{

	}

	bool CharacterControllerSystem::Init()
	{
		auto onCharacterColliderRemoval = std::bind(&CharacterControllerSystem::OnCharacterColliderRemoval, this, std::placeholders::_1);

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_ForeignPlayerEntities,
				.ComponentAccesses =
				{
					{RW, CharacterColliderComponent::Type()},
					{RW, NetworkPositionComponent::Type()},
					{RW, VelocityComponent::Type()},
					{NDA, PlayerForeignComponent::Type()}
				},
				.OnEntityRemoval = onCharacterColliderRemoval
			}
		};
		systemReg.Phase = 1;

		RegisterSystem(systemReg);

		SetComponentOwner<CharacterColliderComponent>({ std::bind(&CharacterControllerSystem::CharacterColliderDestructor, this, std::placeholders::_1) });

		return false;
	}

	void CharacterControllerSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	/*
	* Only called on the client side
	*/
	void CharacterControllerSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		const float32 dt = (float32)deltaTime.AsSeconds();
		TickCharacterControllers(dt);
	}

	void CharacterControllerSystem::TickCharacterControllers(float32 dt)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		auto* pCharacterColliders		= pECS->GetComponentArray<CharacterColliderComponent>();
		auto* pPositionComponents		= pECS->GetComponentArray<PositionComponent>();
		auto* pVelocityComponents		= pECS->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_ForeignPlayerEntities)
		{
			CharacterColliderComponent& characterCollider	= pCharacterColliders->GetData(entity);
			const PositionComponent& positionComp			= pPositionComponents->GetData(entity);
			VelocityComponent& velocityComp					= pVelocityComponents->GetData(entity);

			glm::vec3& velocity			= velocityComp.Velocity;
			const glm::vec3& position	= positionComp.Position;

			velocity.y -= GRAVITATIONAL_ACCELERATION * dt;

			PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
			translationPX *= dt;

			PxController* pController = characterCollider.pController;

			const PxExtendedVec3 oldPositionPX = pController->getPosition();

			if (positionComp.Dirty)
			{
				// Distance between the capsule's feet to its center position. Includes contact offset.
				const float32 capsuleHalfHeight = float32(oldPositionPX.y - pController->getFootPosition().y);
				pController->setPosition({ position.x, position.y - characterHeight + capsuleHalfHeight, position.z });
			}

			pController->move(translationPX, 0.0f, dt, characterCollider.Filters);

			const PxExtendedVec3& newPositionPX = pController->getPosition();
			velocity = {
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
			}

			//Maybe add something to change the rendered PositionComponent here in case we collide
		}
	}

	/*
	* Sets the position of the PxController taken from the PositionComponent.
	* Move the PxController using the VelocityComponent by the deltatime.
	* Calculates a new Velocity based on the difference of the last position and the new one.
	* Sets the new position of the PositionComponent
	*/
	void CharacterControllerSystem::TickCharacterController(float32 dt, Entity entity, ComponentArray<CharacterColliderComponent>* pCharacterColliders, ComponentArray<NetworkPositionComponent>* pNetPosComponents, ComponentArray<VelocityComponent>* pVelocityComponents)
	{
		CharacterColliderComponent& characterCollider = pCharacterColliders->GetData(entity);
		const NetworkPositionComponent& positionComp = pNetPosComponents->GetData(entity);

		glm::vec3& velocity = pVelocityComponents->GetData(entity).Velocity;
		const glm::vec3& position = positionComp.Position;

		velocity.y -= GRAVITATIONAL_ACCELERATION * dt;

		PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
		translationPX *= dt;

		PxController* pController = characterCollider.pController;

		const PxExtendedVec3 oldPositionPX = pController->getPosition();

		if (positionComp.Dirty)
		{
			// Distance between the capsule's feet to its center position. Includes contact offset.
			const float32 capsuleHalfHeight = float32(oldPositionPX.y - pController->getFootPosition().y);
			pController->setPosition({ position.x, position.y - characterHeight + capsuleHalfHeight, position.z });
		}

		pController->move(translationPX, 0.0f, dt, characterCollider.Filters);

		const PxExtendedVec3& newPositionPX = pController->getPosition();
		velocity = {
			(float)newPositionPX.x - oldPositionPX.x,
			(float)newPositionPX.y - oldPositionPX.y,
			(float)newPositionPX.z - oldPositionPX.z
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
			positionCompMutable.Dirty = true;

			positionCompMutable.Position = {
				newPositionPX.x,
				pController->getFootPosition().y + characterHeight,
				newPositionPX.z
			};
		}
	}

	void CharacterControllerSystem::OnCharacterColliderRemoval(Entity entity)
	{
		CharacterColliderComponent& characterCollider = ECSCore::GetInstance()->GetComponent<CharacterColliderComponent>(entity);
		PxActor* pActor = characterCollider.pController->getActor();
		if (pActor)
		{
			PhysicsSystem::GetInstance()->GetScene()->removeActor(*pActor);
		}
	}

	void CharacterControllerSystem::CharacterColliderDestructor(CharacterColliderComponent& characterColliderComponent)
	{
		PhysicsSystem::GetInstance()->GetScene()->removeActor(*characterColliderComponent.pController->getActor());
		if (characterColliderComponent.pController)
		{
			characterColliderComponent.pController->release();
			characterColliderComponent.pController = nullptr;
		}
		SAFEDELETE(characterColliderComponent.Filters.mFilterData);
	}
}