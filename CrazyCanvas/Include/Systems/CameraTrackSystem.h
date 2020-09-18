#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

struct CameraTrackComponent
{
	DECL_COMPONENT(CameraTrackComponent);
	std::vector<glm::vec3> Track;
	size_t CurrentTrackIndex = 0;
	float32 CurrentTrackT = 0.0f;
};

void CreateCameraTrackEntity(const LambdaEngine::CameraDesc& cameraDesc, const std::vector<glm::vec3>& track);

class CameraTrackSystem : public LambdaEngine::System
{
public:
	DECL_REMOVE_COPY(CameraTrackSystem);
	DECL_REMOVE_MOVE(CameraTrackSystem);
	~CameraTrackSystem() = default;

	bool Init();

	bool Release();

	void Tick(LambdaEngine::Timestamp deltaTime) override;

	bool HasReachedEnd() const { return  m_EndReached; }

public:
	static CameraTrackSystem& GetInstance() { return s_Instance; }

private:
	CameraTrackSystem() = default;

	void UpdateTrack(LambdaEngine::Timestamp deltaTime, LambdaEngine::Entity entity, LambdaEngine::CameraComponent & camComp, LambdaEngine::ViewProjectionMatricesComponent & viewProjComp, LambdaEngine::PositionComponent & posComp, LambdaEngine::RotationComponent & rotComp);
	
	glm::vec3 GetCurrentGradient(const glm::uvec4& splineIndices, CameraTrackComponent& camTrackComp) const;
	glm::uvec4 GetCurrentSplineIndices(CameraTrackComponent& camTrackComp) const;
	bool HasReachedEnd(CameraTrackComponent& camTrackComp) const { return  camTrackComp.CurrentTrackIndex == camTrackComp.Track.size() - 1; }

private:
	LambdaEngine::IDVector	m_CameraEntities;
	bool m_EndReached = false;

private:
	static CameraTrackSystem s_Instance;
};