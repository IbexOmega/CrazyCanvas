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
		auto* pPositionComponents		= pECS->GetComponentArray<NetworkPositionComponent>();
		auto* pVelocityComponents		= pECS->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_ForeignPlayerEntities)
		{
			CharacterColliderComponent& characterCollider	= pCharacterColliders->GetData(entity);
			const NetworkPositionComponent& positionComp	= pPositionComponents->GetConstData(entity);
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

			const PxExtendedVec3& newPositionPX = pController->getFootPosition();
			velocityComp.Velocity = {
				(float)newPositionPX.x - oldPositionPX.x,
				(float)newPositionPX.y - oldPositionPX.y,
				(float)newPositionPX.z - oldPositionPX.z
			};
			velocityComp.Velocity /= dt;

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

			//Maybe add something to change the rendered PositionComponent here in case we collide
		}
	}

	/*
	* Sets the position of the PxController taken from the PositionComponent.
	* Move the PxController using the VelocityComponent by the deltatime.
	* Calculates a new Velocity based on the difference of the last position and the new one.
	* Sets the new position of the PositionComponent
	*/
	void CharacterControllerSystem::TickCharacterController(
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

		//Update velocity
		glm::vec3& velocity = velocityComp.Velocity;
		velocity.y -= GRAVITATIONAL_ACCELERATION * dt;

		//Calculate Tick Translation
		PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
		translationPX *= dt;

		const PxExtendedVec3 oldPositionPX = pController->getFootPosition();

		// Distance between the capsule's feet to its center position. Includes contact offset.
		if (positionComp.Dirty)
		{
			pController->setFootPosition({ positionComp.Position.x,  positionComp.Position.y,  positionComp.Position.z });
		}

		pController->move(translationPX, 0.0f, dt, characterCollider.Filters);

		const PxExtendedVec3& newPositionPX = pController->getFootPosition();
		velocity = 
		{
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

			positionCompMutable.Position = 
			{
				newPositionPX.x,
				newPositionPX.y,
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