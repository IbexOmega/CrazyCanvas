#pragma once

#include "Containers/TArray.h"
#include "ECS/System.h"
#include "Math/Math.h"

class CameraTrack : public LambdaEngine::System
{
public:
	CameraTrack();
	~CameraTrack() = default;

	void SetTrack(const LambdaEngine::TArray<glm::vec3>& track) { m_Track = track; }
	bool InitSystem() { return true; }
	void Tick(float32 dt);

	bool HasReachedEnd() const { return m_CurrentTrackIndex == m_Track.GetSize() - 1; }

private:
	glm::vec3 GetCurrentGradient(const glm::uvec4& splineIndices) const;
	glm::uvec4 GetCurrentSplineIndices() const;

private:
	LambdaEngine::IDVector m_Cameras;

	LambdaEngine::TArray<glm::vec3> m_Track;

	uint32 m_CurrentTrackIndex = 0;
	float32 m_CurrentTrackT = 0.0f;
};
