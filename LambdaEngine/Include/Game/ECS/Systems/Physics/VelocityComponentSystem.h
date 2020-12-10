#pragma once

#include "ECS/System.h"

namespace LambdaEngine
{
	class VelocityComponentSystem : public System
	{
	public:
		VelocityComponentSystem() = default;
		~VelocityComponentSystem() = default;

		void Init();

		void Tick(Timestamp deltaTime) override final;

		static VelocityComponentSystem* GetInstance() { return &s_Instance; }

	private:
		static VelocityComponentSystem s_Instance;

	private:
		IDVector m_VelocityEntities;
	};
}
