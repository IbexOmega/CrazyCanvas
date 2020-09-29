#include "Game/ECS/Systems/Player/PlayerMovementSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Player/ControllableComponent.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Networking/API/PlatformNetworkUtils.h"

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
				{{{R, ControllableComponent::Type()}}, {&transformComponents}, &m_ControllableEntities}
			};
			systemReg.Phase = 0;

			RegisterSystem(systemReg);
		}

		return true;
    }

    void PlayerMovementSystem::Tick(Timestamp deltaTime)
    {
		int8 deltaForward = int8(Input::IsKeyDown(EKey::KEY_T) - Input::IsKeyDown(EKey::KEY_G));
		int8 deltaLeft = int8(Input::IsKeyDown(EKey::KEY_F) - Input::IsKeyDown(EKey::KEY_H));

		for (Entity entity : m_ControllableEntities)
		{
			Move(entity, deltaTime, deltaForward, deltaLeft);
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
			positionComponent.Position.z += 1.0f * deltaTime.AsSeconds() * deltaForward;
			positionComponent.Dirty = true;
		}

		if (deltaLeft != 0)
		{
			positionComponent.Position.x += 1.0f * deltaTime.AsSeconds() * deltaLeft;
			positionComponent.Dirty = true;
		}
	}
}