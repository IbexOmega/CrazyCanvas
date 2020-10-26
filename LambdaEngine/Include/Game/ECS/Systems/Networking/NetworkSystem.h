#pragma once

#include "ECS/System.h"

namespace LambdaEngine
{
	class NetworkSystem : public System
	{
	public:
		NetworkSystem();
		~NetworkSystem();

		bool Init();

	private:
		virtual void Tick(Timestamp deltaTime) override final { UNREFERENCED_VARIABLE(deltaTime); };
		void OnEntityAdded(Entity entity);
		void OnEntityRemoved(Entity entity);

	public:
		static NetworkSystem& GetInstance() { return s_Instance; }

	private:
		IDVector m_Entities;

	private:
		static NetworkSystem s_Instance;
	};
}