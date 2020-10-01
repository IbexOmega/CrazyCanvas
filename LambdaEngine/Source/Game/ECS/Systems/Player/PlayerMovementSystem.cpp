#include "Game/ECS/Systems/Player/PlayerMovementSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Player/ControllableComponent.h"

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
				{{{RW, ControllableComponent::Type()}}, {&transformComponents}, &m_ControllableEntities}
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
			auto* pInterpolationComponents	= pECS->GetComponentArray<ControllableComponent>();

			if (!pInterpolationComponents)
				return;

			ControllableComponent& controllableComponent = pInterpolationComponents->GetData(entity);

			controllableComponent.StartPosition = controllableComponent.EndPosition;
			InterpolationMove(deltaTime, deltaForward, deltaLeft, controllableComponent.EndPosition);
			controllableComponent.StartTimestamp = EngineLoop::GetTimeSinceStart();
		}
	}

	void PlayerMovementSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);

		ECSCore* pECS = ECSCore::GetInstance();
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
			positionComponent.Dirty = true;
		}
	}

	void PlayerMovementSystem::Move(Entity entity, Timestamp deltaTime, int8 deltaForward, int8 deltaLeft)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

		if (!pPositionComponents && !pPositionComponents->HasComponent(entity))
			return;

		PositionComponent& positionComponent = pPositionComponents->GetData(entity);

		if (deltaForward != 0)
		{
			positionComponent.Position.z += (float32)((1.0 * deltaTime.AsSeconds()) * (float64)deltaForward);
			positionComponent.Dirty = true;
		}

		if (deltaLeft != 0)
		{
			positionComponent.Position.x += (float32)((1.0 * deltaTime.AsSeconds()) * (float64)deltaLeft);
			positionComponent.Dirty = true;
		}
	}

	void PlayerMovementSystem::InterpolationMove( Timestamp deltaTime, int8 deltaForward, int8 deltaLeft, glm::vec3& result)
	{
		if (deltaForward != 0)
		{
			result.z += (float32)((1.0f * deltaTime.AsSeconds()) * (float64)deltaForward);
		}

		if (deltaLeft != 0)
		{
			result.x += (float32)((1.0f * deltaTime.AsSeconds()) * (float64)deltaLeft);
		}
	}
	
	void PlayerMovementSystem::Interpolate(const glm::vec3& start, const glm::vec3& end, glm::vec3& result, float32 percentage)
	{
		result.x = (end.x - start.x) * percentage + start.x;
		result.y = (end.y - start.y) * percentage + start.y;
		result.z = (end.z - start.z) * percentage + start.z;
	}
}