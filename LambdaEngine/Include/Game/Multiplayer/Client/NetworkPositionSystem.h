#pragma once
#include "ECS/System.h"

#include "Types.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	class NetworkPositionSystem : public System
	{
		friend class ClientSystem;

	public:
		DECL_UNIQUE_CLASS(NetworkPositionSystem);
		virtual ~NetworkPositionSystem() = default;

	private:
		NetworkPositionSystem() = default;

		void Init();

		void Tick(Timestamp deltaTime) override;

		static void Interpolate(const glm::vec3& start, const glm::vec3& end, glm::vec3& result, float32 percentage);

	private:
		IDVector m_Entities;
	};
}