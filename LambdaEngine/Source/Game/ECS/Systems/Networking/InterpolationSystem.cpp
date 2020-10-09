#include "Game/ECS/Systems/Networking/InterpolationSystem.h"
#include "Game/ECS/Systems/Networking/ClientSystem.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineLoop.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Networking/InterpolationComponent.h"

namespace LambdaEngine
{
	InterpolationSystem::InterpolationSystem() :
		m_InterpolationEntities()
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_InterpolationEntities,
				.ComponentAccesses =
				{
					{RW, PositionComponent::Type()}, {RW, InterpolationComponent::Type()}
				}
			}
		};
		systemReg.Phase = 0;
		RegisterSystem(systemReg);

		ClientSystem::GetInstance().SubscribeToPacketType(NetworkSegment::TYPE_PLAYER_ACTION, std::bind(&InterpolationSystem::OnPacketPlayerAction, this, std::placeholders::_1));
	}

	InterpolationSystem::~InterpolationSystem()
	{

	}

	void InterpolationSystem::Tick(Timestamp deltaTime)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		auto* pInterpolationComponents	= pECS->GetComponentArray<InterpolationComponent>();
		auto* pPositionComponents		= pECS->GetComponentArray<PositionComponent>();

		Timestamp currentTime	= EngineLoop::GetTimeSinceStart();
		float64 percentage;

		for (auto& entity : m_InterpolationEntities)
		{
			InterpolationComponent& interpolationComponent	= pInterpolationComponents->GetData(entity);
			PositionComponent& positionComponent			= pPositionComponents->GetData(entity);

			deltaTime = currentTime - interpolationComponent.StartTimestamp;
			percentage = deltaTime.AsSeconds() / interpolationComponent.Duration.AsSeconds();
			percentage = percentage > 1.0f ? 1.0f : percentage < 0.0f ? 0.0f : percentage;

			Interpolate(interpolationComponent.StartPosition, interpolationComponent.EndPosition, positionComponent.Position, (float32)percentage);

		}
	}

	void InterpolationSystem::OnPacketPlayerAction(NetworkSegment* pPacket)
	{
		const ClientSystem& clientSystem = ClientSystem::GetInstance();

		BinaryDecoder decoder(pPacket);
		int32 networkUID				= decoder.ReadInt32();
		int32 simulationtick			= decoder.ReadInt32();

		if (!clientSystem.IsLocalClient(networkUID))
		{
			ECSCore* pECS = ECSCore::GetInstance();
			auto* pInterpolationComponents = pECS->GetComponentArray<InterpolationComponent>();

			if (!pInterpolationComponents)
				return;

			InterpolationComponent& interpolationComponent = pInterpolationComponents->GetData(clientSystem.GetEntityFromNetworkUID(networkUID));

			interpolationComponent.StartPosition	= interpolationComponent.EndPosition;
			interpolationComponent.EndPosition		= decoder.ReadVec3();
			interpolationComponent.StartTimestamp	= EngineLoop::GetTimeSinceStart();
		}
	}

	void InterpolationSystem::Interpolate(const glm::vec3& start, const glm::vec3& end, glm::vec3& result, float32 percentage)
	{
		result.x = (end.x - start.x) * percentage + start.x;
		result.y = (end.y - start.y) * percentage + start.y;
		result.z = (end.z - start.z) * percentage + start.z;
	}
}
