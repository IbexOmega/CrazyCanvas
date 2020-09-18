#include "CameraTrack.h"

#include "Game/Camera.h"

#include <algorithm>

bool CameraTrack::Init(LambdaEngine::Camera* pCamera, const LambdaEngine::TArray<glm::vec3>& track)
{
	m_pCamera = pCamera;
	m_Track = track;
	return pCamera;
}

void CameraTrack::Tick(float32 dt)
{
	if (HasReachedEnd())
	{
		return;
	}

	constexpr const float cameraSpeed = 1.4f;
	glm::uvec4 splineIndices = GetCurrentSplineIndices();
	const float tPerSecond = cameraSpeed / glm::length(GetCurrentGradient(splineIndices));

	m_CurrentTrackT += dt * tPerSecond;
	m_CurrentTrackIndex += std::min(1u, (uint32)m_CurrentTrackT);
	splineIndices = GetCurrentSplineIndices();
	m_CurrentTrackT = std::modf(m_CurrentTrackT, &m_CurrentTrackT); // Remove integer part

	if (HasReachedEnd())
	{
		return;
	}

	glm::vec3 oldPos = m_pCamera->GetPosition();
	m_pCamera->SetPosition(glm::catmullRom(
		m_Track[splineIndices.x],
		m_Track[splineIndices.y],
		m_Track[splineIndices.z],
		m_Track[splineIndices.w],
		m_CurrentTrackT));

	m_pCamera->SetDirection(glm::normalize(GetCurrentGradient(splineIndices)));
}

glm::vec3 CameraTrack::GetCurrentGradient(const glm::uvec4& splineIndices) const
{
	const float tt = m_CurrentTrackT * m_CurrentTrackT;

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
