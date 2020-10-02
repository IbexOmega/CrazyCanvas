#pragma once

#include "ECS/System.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class InterpolationSystem : public System
	{
		friend class ClientSystem;

	public:
		DECL_UNIQUE_CLASS(InterpolationSystem);
		virtual ~InterpolationSystem();

		void Tick(Timestamp deltaTime) override;

	private:
		InterpolationSystem();

		void OnPacketPlayerAction(NetworkSegment* pPacket);

		void Interpolate(const glm::vec3& start, const glm::vec3& end, glm::vec3& result, float32 percentage);

	private:
		IDVector m_InterpolationEntities;
	};
}