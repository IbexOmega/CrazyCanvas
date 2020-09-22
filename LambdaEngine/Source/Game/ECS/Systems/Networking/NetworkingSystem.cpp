#include "Game/ECS/Systems/Networking/NetworkingSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

namespace LambdaEngine
{
	NetworkingSystem NetworkingSystem::s_Instance;

	bool NetworkingSystem::Init()
	{
		TransformComponents transformComponents;
		transformComponents.Position.Permissions = RW;
		transformComponents.Scale.Permissions = R;
		transformComponents.Rotation.Permissions = R;

		// Subscribe on entities with transform and viewProjectionMatrices. They are considered the camera.
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{RW, NetworkComponent::s_TID}}, {&transformComponents}, &m_NetworkEntities}
			};
			systemReg.Phase = 0;

			RegisterSystem(systemReg);
		}

		return true;
	}

	void NetworkingSystem::Tick(Timestamp deltaTime)
	{
		/*ECSCore* pECS = ECSCore::GetInstance();

		auto* pComponents = pECS->GetComponentArray<PositionComponent>();

		for (Entity entity : m_NetworkEntities)
		{
			auto& positionComponent = pComponents->GetData(entity);
			positionComponent.Position.x += 0.1f * deltaTime.AsSeconds();
			positionComponent.Dirty = true;
		}*/
	}
}
