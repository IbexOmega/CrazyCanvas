#pragma once

#include "ECS/ComponentOwner.h"
#include "ECS/System.h"

namespace LambdaEngine
{
	struct NetworkComponent;

	class NetworkSystem : ComponentOwner
	{
	public:
		NetworkSystem() = default;
		~NetworkSystem() = default;

		bool Init();

	public:
		static NetworkSystem& GetInstance() { return s_Instance; }

	private:
		static void NetworkComponentConstructor(NetworkComponent& networkComponent, Entity entity);
		static void NetworkComponentDestructor(NetworkComponent& networkComponent, Entity entity);

	private:
		IDVector m_Entities;

	private:
		static NetworkSystem s_Instance;
	};
}