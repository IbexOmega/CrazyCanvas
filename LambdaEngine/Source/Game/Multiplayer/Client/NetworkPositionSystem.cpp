#include "Game/Multiplayer/Client/NetworkPositionSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	NetworkPositionSystem::NetworkPositionSystem()
	{

	}

	NetworkPositionSystem::~NetworkPositionSystem()
	{

	}

	void NetworkPositionSystem::Init()
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_Entities,
				.ComponentAccesses =
				{
					{RW, PositionComponent::Type()}, 
					{RW, NetworkPositionComponent::Type()} 
				}
			}
		};
		systemReg.Phase = 0;

		RegisterSystem(systemReg);
	}

	void NetworkPositionSystem::Tick(Timestamp deltaTime)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<NetworkPositionComponent>* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
		ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

		Timestamp currentTime = EngineLoop::GetTimeSinceStart();
		float64 percentage;

		for (Entity entity : m_Entities)
		{
			const NetworkPositionComponent& netPosComponent	= pNetPosComponents->GetConstData(entity);
			const PositionComponent& constPositionComponent = pPositionComponents->GetConstData(entity);

			if (glm::any(glm::notEqual(netPosComponent.Position, constPositionComponent.Position)))
			{
				deltaTime = currentTime - netPosComponent.TimestampStart;
				percentage = deltaTime.AsSeconds() / netPosComponent.Duration.AsSeconds();
				percentage = glm::clamp<float64>(percentage, 0.0, 1.0);

				PositionComponent& positionComponent = const_cast<PositionComponent&>(constPositionComponent);
				Interpolate(netPosComponent.PositionLast, netPosComponent.Position, positionComponent.Position, (float32)percentage);
				positionComponent.Dirty = true;
			}
		}
	}

	void NetworkPositionSystem::Interpolate(const glm::vec3& start, const glm::vec3& end, glm::vec3& result, float32 percentage)
	{
		result.x = (end.x - start.x) * percentage + start.x;
		result.y = (end.y - start.y) * percentage + start.y;
		result.z = (end.z - start.z) * percentage + start.z;
	}
}