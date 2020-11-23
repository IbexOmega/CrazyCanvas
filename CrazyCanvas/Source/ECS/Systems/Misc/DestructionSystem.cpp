#include "ECS/Systems/Misc/DestructionSystem.h"

#include "ECS/Components/Misc/DestructionComponent.h"
#include "ECS/ECSCore.h"

DestructionSystem::DestructionSystem()
{
}

DestructionSystem::~DestructionSystem()
{
}

bool DestructionSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_DestructionEntities,
				.ComponentAccesses =
				{
					{RW, DestructionComponent::Type()}
				},
			}
		};
		systemReg.Phase = 1u;

		RegisterSystem(TYPE_NAME(DestructionSystem), systemReg);
	}

	return true;
}

void DestructionSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<DestructionComponent>* pDestructionComponents = pECS->GetComponentArray<DestructionComponent>();

	for (Entity entity : m_DestructionEntities)
	{
		DestructionComponent& destructionComp = pDestructionComponents->GetData(entity);

		if (destructionComp.Active)
		{
			destructionComp.TimeLeft -= float(deltaTime.AsSeconds());

			if (destructionComp.TimeLeft <= 0.0f)
			{
				// Destroy Entity
				pECS->RemoveEntity(entity);
			}
		}
	}
}
