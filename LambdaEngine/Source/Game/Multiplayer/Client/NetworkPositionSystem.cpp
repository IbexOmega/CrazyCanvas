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
		auto* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

		Timestamp currentTime = EngineLoop::GetTimeSinceStart();
		float64 percentage;

		for (auto& entity : m_Entities)
		{
			NetworkPositionComponent& netPosComponent = pNetPosComponents->GetData(entity);
			PositionComponent& positionComponent = pPositionComponents->GetData(entity);

			deltaTime = currentTime - netPosComponent.TimestampStart;
			percentage = deltaTime.AsSeconds() / netPosComponent.Duration.AsSeconds();
			percentage = percentage > 1.0f ? 1.0f : percentage < 0.0f ? 0.0f : percentage;

			Interpolate(netPosComponent.PositionLast, netPosComponent.Position, positionComponent.Position, (float32)percentage);
		}
	}

	void NetworkPositionSystem::Interpolate(const glm::vec3& start, const glm::vec3& end, glm::vec3& result, float32 percentage)
	{
		result.x = (end.x - start.x) * percentage + start.x;
		result.y = (end.y - start.y) * percentage + start.y;
		result.z = (end.z - start.z) * percentage + start.z;
	}
}