#include "Systems/CameraTrackSystem.h"

#include "ECS/ECSCore.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"
#include "Math/Random.h"

using namespace LambdaEngine;

INIT_COMPONENT(CameraTrackComponent);

bool CameraTrackSystem::Init()
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
			{{{RW, CameraComponent::s_TID}, {RW, ViewProjectionMatricesComponent::s_TID}, {RW, CameraTrackComponent::s_TID}}, {&transformComponents}, &m_CameraEntities}
		};
		systemReg.Phase = g_LastPhase - 1;

		RegisterSystem(systemReg);
	}

	return true;
}

bool CameraTrackSystem::Release()
{
	return false;
}

void CameraTrackSystem::Tick(Timestamp deltaTime)
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

void CameraTrackSystem::UpdateTrack(Timestamp deltaTime, Entity entity, CameraComponent& camComp, ViewProjectionMatricesComponent& viewProjComp, PositionComponent& posComp, RotationComponent& rotComp)
{
	auto& camTrackComp = ECSCore::GetInstance()->GetComponent<CameraTrackComponent>(entity);

	if (HasReachedEnd(camTrackComp))
	{
		m_EndReached = true;
		return;
	}

	constexpr const float cameraSpeed = 1.4f;
	glm::uvec4 splineIndices = GetCurrentSplineIndices(camTrackComp);
	const float tPerSecond = cameraSpeed / glm::length(GetCurrentGradient(splineIndices, camTrackComp));

	camTrackComp.CurrentTrackT += deltaTime.AsSeconds() * tPerSecond;
	camTrackComp.CurrentTrackIndex += std::min(1ULL, (size_t)camTrackComp.CurrentTrackT);
	splineIndices = GetCurrentSplineIndices(camTrackComp);
	camTrackComp.CurrentTrackT = std::modf(camTrackComp.CurrentTrackT, &camTrackComp.CurrentTrackT); // Remove integer part

	if (HasReachedEnd(camTrackComp))
	{
		m_EndReached = true;
		return;
	}

	glm::vec3 oldPos = posComp.Position;
	posComp.Position = glm::catmullRom(
		camTrackComp.Track[splineIndices.x],
		camTrackComp.Track[splineIndices.y],
		camTrackComp.Track[splineIndices.z],
		camTrackComp.Track[splineIndices.w],
		camTrackComp.CurrentTrackT);

	SetForward(rotComp.Quaternion, glm::normalize(GetCurrentGradient(splineIndices, camTrackComp)));
}

glm::vec3 CameraTrackSystem::GetCurrentGradient(const glm::uvec4& splineIndices, CameraTrackComponent& camTrackComp) const
{
	const float tt = camTrackComp.CurrentTrackT * camTrackComp.CurrentTrackT;
	const float ttt = tt * camTrackComp.CurrentTrackT;

	const float weight1 = -3.0f * tt + 4.0f * camTrackComp.CurrentTrackT - 1.0f;
	const float weight2 = 9.0f * tt - 10.0f * camTrackComp.CurrentTrackT;
	const float weight3 = -9.0f * tt + 8.0f * camTrackComp.CurrentTrackT + 1.0f;
	const float weight4 = 3.0f * tt - 2.0f * camTrackComp.CurrentTrackT;

	return 0.5f * (camTrackComp.Track[splineIndices[0]] * weight1 + camTrackComp.Track[splineIndices[1]] * weight2 + camTrackComp.Track[splineIndices[2]] * weight3 + camTrackComp.Track[splineIndices[3]] * weight4);
}

glm::uvec4 CameraTrackSystem::GetCurrentSplineIndices(CameraTrackComponent& camTrackComp) const
{
	return {
		std::max(0, (int)camTrackComp.CurrentTrackIndex - 1),
		camTrackComp.CurrentTrackIndex,
		std::min(camTrackComp.Track.size() - 1, camTrackComp.CurrentTrackIndex + 1),
		std::min(camTrackComp.Track.size() - 1, camTrackComp.CurrentTrackIndex + 2)
	};
}

void CreateCameraTrackEntity(const LambdaEngine::CameraDesc& cameraDesc, const std::vector<glm::vec3>& track)
{
	ECSCore* pECS = ECSCore::GetInstance();
	Entity entity = pECS->CreateEntity();

	CameraTrackComponent camTrackComp;
	camTrackComp.Track = track;
	pECS->AddComponent<CameraTrackComponent>(entity, camTrackComp);

	ViewProjectionMatricesComponent viewProjComp;
	viewProjComp.Projection = glm::perspective(glm::radians(cameraDesc.FOVDegrees), cameraDesc.Width / cameraDesc.Height, cameraDesc.NearPlane, cameraDesc.FarPlane);
	viewProjComp.View = glm::lookAt(cameraDesc.Position, cameraDesc.Position + cameraDesc.Direction, g_DefaultUp);
	pECS->AddComponent<ViewProjectionMatricesComponent>(entity, viewProjComp);

	CameraComponent camComp;
	pECS->AddComponent<CameraComponent>(entity, camComp);

	pECS->AddComponent<PositionComponent>(entity, PositionComponent{ .Position = cameraDesc.Position });
	pECS->AddComponent<RotationComponent>(entity, RotationComponent{ .Quaternion = glm::quatLookAtLH(cameraDesc.Direction, g_DefaultUp) });
	pECS->AddComponent<ScaleComponent>(entity, ScaleComponent{ .Scale = {1.f, 1.f, 1.f} });
}
