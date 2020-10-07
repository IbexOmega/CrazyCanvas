#include "Game/ECS/Systems/Physics/CharacterControllerSystem.h"
#include "Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"

#include <PxPhysicsAPI.h>

namespace LambdaEngine
{
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
				{
					{RW, CharacterColliderComponent::Type()}, {RW, PositionComponent::Type()}, {RW, VelocityComponent::Type()}
				},
				&m_CharacterColliderEntities,
				nullptr,
				onCharacterColliderRemoval
			}
		};
		systemReg.Phase = 1;

		RegisterSystem(systemReg);

		SetComponentOwner<CharacterColliderComponent>({ std::bind(&CharacterControllerSystem::CharacterColliderDestructor, this, std::placeholders::_1) });

		return false;
	}

	void CharacterControllerSystem::Tick(Timestamp deltaTime)
	{
		const float32 dt = (float32)deltaTime.AsSeconds();
		TickCharacterControllers(dt);
	}

	void CharacterControllerSystem::TickCharacterControllers(float32 dt)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<CharacterColliderComponent>* pCharacterColliders = pECS->GetComponentArray<CharacterColliderComponent>();
		ComponentArray<CharacterLocalColliderComponent>* pCharacterLocalColliders = pECS->GetComponentArray<CharacterLocalColliderComponent>();
		ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_CharacterColliderEntities)
		{
			if(!pCharacterLocalColliders->HasComponent(entity))
				TickCharacterController(dt, entity, pCharacterColliders, pPositionComponents, pVelocityComponents);
		}
	}

	/*
	* Sets the position of the PxController taken from the PositionComponent.
	* Move the PxController using the VelocityComponent by the deltatime.
	* Calculates a new Velocity based on the difference of the last position and the new one.
	*/
	void CharacterControllerSystem::TickCharacterController(float32 dt, Entity entity, ComponentArray<CharacterColliderComponent>* pCharacterColliders, ComponentArray<PositionComponent>* pPositionComponents, ComponentArray<VelocityComponent>* pVelocityComponents)
	{
		CharacterColliderComponent& characterCollider = pCharacterColliders->GetData(entity);
		const PositionComponent& positionComp = pPositionComponents->GetData(entity);

		glm::vec3& velocity = pVelocityComponents->GetData(entity).Velocity;
		const glm::vec3& position = positionComp.Position;

		PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
		translationPX *= dt;

		PxController* pController = characterCollider.pController;

		pController->setPosition({ position.x, position.y, position.z });
		pController->move(translationPX, 0.0f, dt, characterCollider.Filters);

		const PxExtendedVec3& newPositionPX = pController->getPosition();
		velocity = {
			(float)newPositionPX.x - position.x,
			(float)newPositionPX.y - position.y,
			(float)newPositionPX.z - position.z
		};
		velocity /= dt;

		if (glm::length2(velocity) > glm::epsilon<float32>())
		{
			PositionComponent& positionCompMutable = const_cast<PositionComponent&>(positionComp);

			positionCompMutable.Position = {
				(float)newPositionPX.x,
				(float)newPositionPX.y,
				(float)newPositionPX.z
			};

			positionCompMutable.Dirty = true;
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