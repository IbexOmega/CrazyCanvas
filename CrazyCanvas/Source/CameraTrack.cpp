#include "CameraTrack.h"

#include "Game/ECS/Rendering/ComponentGroups.h"

#include <algorithm>

CameraTrack::CameraTrack()
{
	using namespace LambdaEngine;
	CameraComponents cameraComponents;
	cameraComponents.Position.Permissions				= RW;
	cameraComponents.Rotation.Permissions				= RW;
	cameraComponents.ViewProjectionMatrices.Permissions	= NDA;
	cameraComponents.CameraProperties.Permissions		= NDA;

	SystemRegistration sysReg = {};
	sysReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{{&cameraComponents}, &m_Cameras},
	};
}

void CameraTrack::Tick(float32 dt)
{
	if (m_Cameras.Empty() || HasReachedEnd())
	{
		return;
	}

	LambdaEngine::TransformHandler* pTransformHandler = LambdaEngine::TransformHandler::GetInstance();

	constexpr const float cameraSpeed = 1.4f;
	glm::uvec4 splineIndices = GetCurrentSplineIndices();
	const float tPerSecond = cameraSpeed / glm::length(GetCurrentGradient(splineIndices));

	m_CurrentTrackT += dt * tPerSecond;
	m_CurrentTrackIndex += (size_t)m_CurrentTrackT;
	splineIndices = GetCurrentSplineIndices();
	m_CurrentTrackT = std::modf(m_CurrentTrackT, &m_CurrentTrackT); // Remove integer part

	if (HasReachedEnd())
	{
		return;
	}

	glm::vec3& position = pTransformHandler->GetPosition(m_Cameras[0]);
	glm::vec3 oldPos = position;
	position = glm::catmullRom(
		m_Track[splineIndices.x],
		m_Track[splineIndices.y],
		m_Track[splineIndices.z],
		m_Track[splineIndices.w],
		m_CurrentTrackT);

	glm::quat& rotation = pTransformHandler->GetRotation(m_Cameras[0]);
	LambdaEngine::TransformHandler::SetForward(rotation, glm::normalize(GetCurrentGradient(splineIndices)));
}

glm::vec3 CameraTrack::GetCurrentGradient(const glm::uvec4& splineIndices) const
{
	const float tt = m_CurrentTrackT * m_CurrentTrackT;
	const float ttt = tt * m_CurrentTrackT;

	const float weight1 = -3.0f * tt + 4.0f * m_CurrentTrackT - 1.0f;
	const float weight2 = 9.0f * tt - 10.0f * m_CurrentTrackT;
	const float weight3 = -9.0f * tt + 8.0f * m_CurrentTrackT + 1.0f;
	const float weight4 = 3.0f * tt - 2.0f * m_CurrentTrackT;

	return 0.5f * (m_Track[splineIndices[0]] * weight1 + m_Track[splineIndices[1]] * weight2 + m_Track[splineIndices[2]] * weight3 + m_Track[splineIndices[3]] * weight4);
}

glm::uvec4 CameraTrack::GetCurrentSplineIndices() const
{
	return {
		std::max(0, (int)m_CurrentTrackIndex - 1),
		m_CurrentTrackIndex,
		std::min(m_Track.GetSize() - 1, m_CurrentTrackIndex + 1),
		std::min(m_Track.GetSize() - 1, m_CurrentTrackIndex + 2)
	};
}
