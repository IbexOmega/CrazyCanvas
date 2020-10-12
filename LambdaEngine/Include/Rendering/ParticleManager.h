#pragma once

#include "ECS/ECSCore.h"

namespace LambdaEngine {

	class ParticleManager
	{
	public:
		ParticleManager() = default;
		~ParticleManager() = default;

		void Init();
		void Release();

		void OnEmitterEntityAdded(Entity entity);
		void OnEmitterEntityRemoved(Entity entity);

	private:

	};
}