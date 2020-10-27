#include "Game/ECS/Systems/TrackSystem.h"

#include "ECS/ECSCore.h"

using namespace LambdaEngine;

TrackSystem TrackSystem::s_Instance;

bool TrackSystem::Init()
{
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_CameraEntities,
				.ComponentAccesses =
				{
					{RW, TrackComponent::Type()}, {RW, PositionComponent::Type()}, {RW, RotationComponent::Type()}
				}
			}
		};
		systemReg.Phase = 0;

		RegisterSystem(TYPE_NAME(TrackSystem), systemReg);
	}

	return true;
}

void TrackSystem::Tick(Timestamp deltaTime)
{
	ECSCore* pECSCore = ECSCore::GetInstance();

	auto* pPosComps		= pECSCore->GetComponentArray<PositionComponent>();
	auto* pRotComps		= pECSCore->GetComponentArray<RotationComponent>();

	for (Entity entity : m_CameraEntities)
	{
		auto& trackComp = ECSCore::GetInstance()->GetComponent<TrackComponent>(entity);

		if (HasReachedEnd(trackComp))
		{
			trackComp.HasReachedEnd = true;
			return;
		}

		constexpr const float cameraSpeed = 1.4f;
		glm::uvec4 splineIndices = GetCurrentSplineIndices(trackComp);
		const float tPerSecond = cameraSpeed / glm::length(GetCurrentGradient(splineIndices, trackComp));

		trackComp.CurrentTrackT += float32(deltaTime.AsSeconds()) * tPerSecond;
		trackComp.CurrentTrackIndex += std::min<uint32>(1, (uint32)trackComp.CurrentTrackT);
		splineIndices = GetCurrentSplineIndices(trackComp);
		trackComp.CurrentTrackT = std::modf(trackComp.CurrentTrackT, &trackComp.CurrentTrackT); // Remove integer part

		if (HasReachedEnd(trackComp))
		{
			trackComp.HasReachedEnd = true;
			return;
		}

		PositionComponent& posComp = pPosComps->GetData(entity);
		posComp.Position = glm::catmullRom(
			trackComp.Track[splineIndices.x],
			trackComp.Track[splineIndices.y],
			trackComp.Track[splineIndices.z],
			trackComp.Track[splineIndices.w],
			trackComp.CurrentTrackT);

		RotationComponent& rotComp = pRotComps->GetData(entity);
		const glm::vec3 dir = glm::normalize(GetCurrentGradient(splineIndices, trackComp));
		rotComp.Quaternion = glm::quatLookAt(dir, g_DefaultUp);
	}
}

bool TrackSystem::HasReachedEnd(LambdaEngine::Entity entity) const
{
	return ECSCore::GetInstance()->GetComponent<TrackComponent>(entity).HasReachedEnd;
}

glm::vec3 TrackSystem::GetCurrentGradient(const glm::uvec4& splineIndices, const TrackComponent& camTrackComp)
{
	const float tt = camTrackComp.CurrentTrackT * camTrackComp.CurrentTrackT;

	const float weight1 = -3.0f * tt + 4.0f * camTrackComp.CurrentTrackT - 1.0f;
	const float weight2 = 9.0f * tt - 10.0f * camTrackComp.CurrentTrackT;
	const float weight3 = -9.0f * tt + 8.0f * camTrackComp.CurrentTrackT + 1.0f;
	const float weight4 = 3.0f * tt - 2.0f * camTrackComp.CurrentTrackT;

	return 0.5f * (camTrackComp.Track[splineIndices[0]] * weight1 + camTrackComp.Track[splineIndices[1]] * weight2 + camTrackComp.Track[splineIndices[2]] * weight3 + camTrackComp.Track[splineIndices[3]] * weight4);
}

glm::uvec4 TrackSystem::GetCurrentSplineIndices(TrackComponent& camTrackComp) const
{
	return {
		std::max(0, (int)camTrackComp.CurrentTrackIndex - 1),
		camTrackComp.CurrentTrackIndex,
		std::min(camTrackComp.Track.GetSize() - 1, camTrackComp.CurrentTrackIndex + 1),
		std::min(camTrackComp.Track.GetSize() - 1, camTrackComp.CurrentTrackIndex + 2)
	};
}

bool LambdaEngine::TrackSystem::HasReachedEnd(const TrackComponent& camTrackComp)
{
	return camTrackComp.CurrentTrackIndex == camTrackComp.Track.GetSize() - 1;
}
