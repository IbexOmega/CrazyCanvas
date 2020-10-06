#include "Game/ECS/Systems/Player/PlayerMovementSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Player/ControllableComponent.h"
#include "Game/ECS/Components/Physics/Collision.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PlayerMovementSystem PlayerMovementSystem::s_Instance;

    bool PlayerMovementSystem::Init()
    {
		TransformComponents transformComponents;
		transformComponents.Position.Permissions	= RW;
		transformComponents.Scale.Permissions		= R;
		transformComponents.Rotation.Permissions	= RW;

		// Subscribe on entities with transform and viewProjectionMatrices. They are considered the camera.
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					{
						{RW, ControllableComponent::Type()},
						{RW, VelocityComponent::Type()},
						{RW, CharacterColliderComponent::Type()},
						{RW, CharacterLocalColliderComponent::Type()}
					},
					{
						&transformComponents
					}, 
					&m_ControllableEntities
				}
			};
			systemReg.Phase = 0;

			RegisterSystem(systemReg);
		}

		return true;
    }

	void PlayerMovementSystem::FixedTick(Timestamp deltaTime)
	{
		int8 deltaForward	= int8(Input::IsKeyDown(EKey::KEY_T) - Input::IsKeyDown(EKey::KEY_G));
		int8 deltaLeft		= int8(Input::IsKeyDown(EKey::KEY_F) - Input::IsKeyDown(EKey::KEY_H));

		for (Entity entity : m_ControllableEntities)
		{
			ECSCore* pECS = ECSCore::GetInstance();
			auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

			if (!pVelocityComponents)
				return;

			VelocityComponent& velocityComponent = pVelocityComponents->GetData(entity);

			PredictVelocity(deltaTime, deltaForward, deltaLeft, velocityComponent.Velocity);
			LOG_MESSAGE("%f, %f, %f", velocityComponent.Velocity.x, velocityComponent.Velocity.y, velocityComponent.Velocity.z);

			/*controllableComponent.StartPosition = controllableComponent.EndPosition;
			PredictMove(deltaTime, deltaForward, deltaLeft, controllableComponent.EndPosition);
			controllableComponent.StartTimestamp = EngineLoop::GetTimeSinceStart();*/
		}
	}

	void PlayerMovementSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);

		/*ECSCore* pECS = ECSCore::GetInstance();
		auto* pControllableComponents	= pECS->GetComponentArray<ControllableComponent>();
		auto* pPositionComponents		= pECS->GetComponentArray<PositionComponent>();

		Timestamp currentTime = EngineLoop::GetTimeSinceStart();
		float64 percentage;

		for (Entity entity : m_ControllableEntities)
		{
			if (!pPositionComponents && !pPositionComponents->HasComponent(entity))
				return;

			ControllableComponent& controllableComponent	= pControllableComponents->GetData(entity);
			PositionComponent& positionComponent			= pPositionComponents->GetData(entity);

			deltaTime = currentTime - controllableComponent.StartTimestamp;
			percentage = deltaTime.AsSeconds() / controllableComponent.Duration.AsSeconds();
			percentage = percentage > 1.0f ? 1.0f : percentage < 0.0f ? 0.0f : percentage;

			Interpolate(controllableComponent.StartPosition, controllableComponent.EndPosition, positionComponent.Position, (float32)percentage);
		}*/
	}

	/*void PlayerMovementSystem::Move(Entity entity, Timestamp deltaTime, int8 deltaForward, int8 deltaLeft)
	{
		using namespace physx;

		ECSCore* pECS = ECSCore::GetInstance();

		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		auto* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
		auto* pCharacterLocalColliderComponents = pECS->GetComponentArray<CharacterLocalColliderComponent>();
		auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		if (!pPositionComponents && !pPositionComponents->HasComponent(entity))
			return;

		const PositionComponent& positionComponent = pPositionComponents->GetData(entity);
		CharacterColliderComponent& characterColliderComponent = pCharacterColliderComponents->GetData(entity);
		CharacterLocalColliderComponent& characterLocalColliderComponent = pCharacterLocalColliderComponents->GetData(entity);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entity);

		const glm::vec3& position = positionComponent.Position;
		glm::vec3& velocity = velocityComponent.Velocity;

		const PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };

		PxController* pController = characterLocalColliderComponent.pController;

		pController->setPosition(characterColliderComponent.pController->getPosition());
		pController->move(translationPX, 0.0f, (float32)deltaTime.AsSeconds(), characterLocalColliderComponent.Filters);

		const PxExtendedVec3& newPositionPX = pController->getPosition();
		velocity = {
			(float)newPositionPX.x - position.x,
			(float)newPositionPX.y - position.y,
			(float)newPositionPX.z - position.z
		};


		//PredictMove(deltaTime, deltaForward, deltaLeft, positionComponent.Position);
	}*/

	void PlayerMovementSystem::PredictVelocity(Timestamp deltaTime, int8 deltaForward, int8 deltaLeft, glm::vec3& result)
	{
		if (deltaForward != 0)
		{
			result.z = (float32)((1.0 * deltaTime.AsSeconds()) * (float64)deltaForward);
		}

		if (deltaLeft != 0)
		{
			result.x = (float32)((1.0 * deltaTime.AsSeconds()) * (float64)deltaLeft);
		}
	}
	
	/*void PlayerMovementSystem::Interpolate(const glm::vec3& start, const glm::vec3& end, glm::vec3& result, float32 percentage)
	{
		result.x = (end.x - start.x) * percentage + start.x;
		result.y = (end.y - start.y) * percentage + start.y;
		result.z = (end.z - start.z) * percentage + start.z;
	}*/
}
