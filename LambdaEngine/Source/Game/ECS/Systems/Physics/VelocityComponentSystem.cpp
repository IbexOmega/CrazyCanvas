#include "Game/ECS/Systems/Physics/VelocityComponentSystem.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"

namespace LambdaEngine
{
	VelocityComponentSystem VelocityComponentSystem::s_Instance;

	void VelocityComponentSystem::Init()
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_VelocityEntities,
				.ComponentAccesses =
				{
					{RW, VelocityComponent::Type()}
				},
			}
		};
		systemReg.Phase = LAST_PHASE;

		RegisterSystem(TYPE_NAME(TransformApplierSystem), systemReg);
	}

	void VelocityComponentSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);

		ECSCore* pECS = ECSCore::GetInstance();

		ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
		for (Entity entity : m_VelocityEntities)
		{
			VelocityComponent& velocityComp = pVelocityComponents->GetData(entity);

			velocityComp.LastNonZeroVelocityComponents.x = glm::epsilonNotEqual<float32>(velocityComp.Velocity.x, 0.0f, glm::epsilon<float32>()) ? velocityComp.Velocity.x : velocityComp.LastNonZeroVelocityComponents.x;
			velocityComp.LastNonZeroVelocityComponents.y = glm::epsilonNotEqual<float32>(velocityComp.Velocity.y, 0.0f, glm::epsilon<float32>()) ? velocityComp.Velocity.y : velocityComp.LastNonZeroVelocityComponents.y;
			velocityComp.LastNonZeroVelocityComponents.z = glm::epsilonNotEqual<float32>(velocityComp.Velocity.z, 0.0f, glm::epsilon<float32>()) ? velocityComp.Velocity.z : velocityComp.LastNonZeroVelocityComponents.z;
		}
	}
}