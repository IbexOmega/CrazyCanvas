#include "Game/ECS/Systems/TrackSystem.h"

#include "ECS/ECSCore.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"
#include "Math/Random.h"

using namespace LambdaEngine;

TrackSystem TrackSystem::s_Instance;

bool TrackSystem::Init()
{
	TransformComponents transformComponents;
	transformComponents.Position.Permissions = R;
	transformComponents.Scale.Permissions = R;
	transformComponents.Rotation.Permissions = R;

	// Subscribe on entities with transform and viewProjectionMatrices. They are considered the camera.
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{{{RW, CameraComponent::s_TID}, {RW, ViewProjectionMatricesComponent::s_TID}, {RW, TrackComponent::s_TID}}, {&transformComponents}, &m_CameraEntities}
		};
		systemReg.Phase = g_LastPhase - 1;

		RegisterSystem(systemReg);
	}

	return true;
}

void TrackSystem::Tick(Timestamp deltaTime)
{
	ECSCore* pECSCore = ECSCore::GetInstance();

	ComponentArray<CameraComponent>* pCameraComponents = pECSCore->GetComponentArray<CameraComponent>();

	for (Entity entity : m_CameraEntities.GetIDs())
	{
		auto& camComp = pCameraComponents->GetData(entity);
		if (camComp.IsActive)
		{
			auto& viewProjComp = pECSCore->GetComponent<ViewProjectionMatricesComponent>(entity);
			auto& posComp = pECSCore->GetComponent<PositionComponent>(entity);
			auto& rotComp = pECSCore->GetComponent<RotationComponent>(entity);

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			uint16 width = window->GetWidth();
			uint16 height = window->GetHeight();
			camComp.Jitter = glm::vec2((Random::Float32() - 0.5f) / (float)width, (Random::Float32() - 0.5f) / (float)height);
			UpdateTrack(deltaTime, entity, camComp, viewProjComp, posComp, rotComp);
		}
	}
}

bool TrackSystem::HasReachedEnd(LambdaEngine::Entity entity) const
{
	return ECSCore::GetInstance()->GetComponent<TrackComponent>(entity).HasReachedEnd;
}

void TrackSystem::UpdateTrack(Timestamp deltaTime, Entity entity, CameraComponent& camComp, ViewProjectionMatricesComponent& viewProjComp, PositionComponent& posComp, RotationComponent& rotComp)
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

	trackComp.CurrentTrackT += deltaTime.AsSeconds() * tPerSecond;
	trackComp.CurrentTrackIndex += std::min(1ULL, (size_t)trackComp.CurrentTrackT);
	splineIndices = GetCurrentSplineIndices(trackComp);
	trackComp.CurrentTrackT = std::modf(trackComp.CurrentTrackT, &trackComp.CurrentTrackT); // Remove integer part

	if (HasReachedEnd(trackComp))
	{
		trackComp.HasReachedEnd = true;
		return;
	}

	glm::vec3 oldPos = posComp.Position;
	posComp.Position = glm::catmullRom(
		trackComp.Track[splineIndices.x],
		trackComp.Track[splineIndices.y],
		trackComp.Track[splineIndices.z],
		trackComp.Track[splineIndices.w],
		trackComp.CurrentTrackT);

	glm::vec3 dir = glm::normalize(GetCurrentGradient(splineIndices, trackComp));
	rotComp.Quaternion = glm::quatLookAtLH(dir, glm::vec3(0.f, -1.f, 0.f)); // Update the Quaternion so that it matches the direction. (This is not used within this system)
	viewProjComp.View = glm::lookAt(posComp.Position, posComp.Position + dir, g_DefaultUp);
	camComp.ViewInv = glm::inverse(viewProjComp.View);
}

glm::vec3 TrackSystem::GetCurrentGradient(const glm::uvec4& splineIndices, TrackComponent& camTrackComp) const
{
	const float tt = camTrackComp.CurrentTrackT * camTrackComp.CurrentTrackT;
	const float ttt = tt * camTrackComp.CurrentTrackT;

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
		std::min(camTrackComp.Track.size() - 1, camTrackComp.CurrentTrackIndex + 1),
		std::min(camTrackComp.Track.size() - 1, camTrackComp.CurrentTrackIndex + 2)
	};
}

bool LambdaEngine::TrackSystem::HasReachedEnd(TrackComponent& camTrackComp) const
{
	return  camTrackComp.CurrentTrackIndex == camTrackComp.Track.size() - 1;
}
