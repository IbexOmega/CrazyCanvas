#pragma once

#include "Math/Math.h"

#include <vector>

namespace LambdaEngine
{
	class Camera;
}

class CameraTrack
{
public:
	CameraTrack() = default;
	~CameraTrack() = default;

	bool Init(LambdaEngine::Camera* pCamera, const std::vector<glm::vec3>& track);
	void Tick(float32 dt);

	bool HasReachedEnd() const { return  m_CurrentTrackIndex == m_Track.size() - 1; }

private:
	glm::vec3 GetCurrentGradient(const glm::uvec4& splineIndices) const;
	glm::uvec4 GetCurrentSplineIndices() const;

private:
	LambdaEngine::Camera* m_pCamera;
	std::vector<glm::vec3> m_Track;

	size_t m_CurrentTrackIndex = 0;
	float32 m_CurrentTrackT = 0.0f;
};
