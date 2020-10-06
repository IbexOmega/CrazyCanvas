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
				{{RW, CharacterColliderComponent::Type()}, {R, PositionComponent::Type()}, {RW, VelocityComponent::Type()}},
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
		const ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_CharacterColliderEntities)
		{
			CharacterColliderComponent& characterCollider = pCharacterColliders->GetData(entity);
			const PositionComponent& positionComp = pPositionComponents->GetData(entity);
			VelocityComponent& velocityComp = pVelocityComponents->GetData(entity);

			TickCharacterController(dt, entity, characterCollider, positionComp, velocityComp);
		}
	}

	void CharacterControllerSystem::TickCharacterController(float32 dt, Entity entity, CharacterColliderComponent& characterCollider, const PositionComponent& positionComp, VelocityComponent& velocityComp)
	{
		const glm::vec3& position = positionComp.Position;
		glm::vec3& velocity = velocityComp.Velocity;

		const PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };

		PxController* pController = characterCollider.pController;

		pController->setPosition({ position.x, position.y, position.z });
		pController->move(translationPX, 0.0f, dt, characterCollider.Filters);

		const PxExtendedVec3& newPositionPX = pController->getPosition();
		velocity = {
			(float)newPositionPX.x - position.x,
			(float)newPositionPX.y - position.y,
			(float)newPositionPX.z - position.z
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