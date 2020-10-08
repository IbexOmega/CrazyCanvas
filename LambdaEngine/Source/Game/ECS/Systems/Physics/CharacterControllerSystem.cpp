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
					{RW, CharacterColliderComponent::Type()}, {RW, NetworkPositionComponent::Type()}, {RW, VelocityComponent::Type()}
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
		/*const float32 dt = (float32)deltaTime.AsSeconds();
		TickCharacterControllers(dt);*/
	}

	void CharacterControllerSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		const float32 dt = (float32)deltaTime.AsSeconds();
		TickCharacterControllers(dt);
	}

	void CharacterControllerSystem::TickCharacterControllers(float32 dt)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		auto* pCharacterColliders = pECS->GetComponentArray<CharacterColliderComponent>();
		auto* pCharacterLocalColliders = pECS->GetComponentArray<CharacterLocalColliderComponent>();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_CharacterColliderEntities)
		{
			if (!pCharacterLocalColliders->HasComponent(entity))
			{
				/*CharacterColliderComponent& characterCollider = pCharacterColliders->GetData(entity);
				const PositionComponent& positionComp = pPositionComponents->GetData(entity);

				glm::vec3& velocity = pVelocityComponents->GetData(entity).Velocity;
				const glm::vec3& position = positionComp.Position;

				PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
				translationPX *= dt;

				PxController* pController = characterCollider.pController;

				pController->setPosition({ position.x, position.y, position.z });
				pController->move(translationPX, 0.0f, dt, characterCollider.Filters);*/

				/*const PxExtendedVec3& newPositionPX = pController->getPosition();
				velocity = {
					(float)newPositionPX.x - position.x,
					(float)newPositionPX.y - position.y,
					(float)newPositionPX.z - position.z
				};
				velocity /= dt;*/

				//Maybe add something to change the rendered PositionComponent here in case we collide
			}
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
		NetworkPositionComponent& positionComp = pNetPosComponents->GetData(entity);

		glm::vec3& velocity = pVelocityComponents->GetData(entity).Velocity;
		glm::vec3& position = positionComp.Position;

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

		position = {
			(float)newPositionPX.x,
			(float)newPositionPX.y,
			(float)newPositionPX.z
		};
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