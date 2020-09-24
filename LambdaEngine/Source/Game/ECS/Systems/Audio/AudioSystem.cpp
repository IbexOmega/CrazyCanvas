#include "Game/ECS/Systems/Audio/AudioSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

namespace LambdaEngine
{
	AudioSystem AudioSystem::s_Instance;

	bool AudioSystem::Init()
	{
		//TransformComponents transformComponents;
		//transformComponents.Position.Permissions = R;
		//transformComponents.Scale.Permissions = R;
		//transformComponents.Rotation.Permissions = R;

		// Subscribe on entities with transform and viewProjectionMatrices. They are considered the camera.
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{R, AudibleComponent::s_TID}, {R, PositionComponent::s_TID}}, {}, &m_AudibleEntities},
				{{{R, AudibleComponent::s_TID}, {R, PositionComponent::s_TID}}, {}, &m_AudibleNoPositionEntities},
			};
			systemReg.Phase = 0;

			RegisterSystem(systemReg);
		}

		return true;
	}

	void AudioSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
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
