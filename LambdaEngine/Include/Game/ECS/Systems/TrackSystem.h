#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Physics/Transform.h"

namespace LambdaEngine
{
	class TrackSystem : public System
	{
	public:
		DECL_REMOVE_COPY(TrackSystem);
		DECL_REMOVE_MOVE(TrackSystem);
		~TrackSystem() = default;

		bool Init();

		void Tick(Timestamp deltaTime) override;

		bool HasReachedEnd(Entity entity) const;

	public:
		static TrackSystem& GetInstance() { return s_Instance; }

	private:
		TrackSystem() = default;

		static glm::vec3 GetCurrentGradient(const glm::uvec4& splineIndices, const TrackComponent& camTrackComp);
		glm::uvec4 GetCurrentSplineIndices(TrackComponent& camTrackComp) const;
		static bool HasReachedEnd(const TrackComponent& camTrackComp);

	private:
		IDVector	m_CameraEntities;
		bool		m_EndReached = false;

	private:
		static TrackSystem s_Instance;
	};
}