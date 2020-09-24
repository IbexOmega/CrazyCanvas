#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

namespace LambdaEngine
{
	class NetworkingSystem : public System
	{
	public:
		DECL_UNIQUE_CLASS(NetworkingSystem);
		~NetworkingSystem() = default;

		bool Init();

		void Tick(Timestamp deltaTime) override;

	public:
		static NetworkingSystem& GetInstance() { return s_Instance; }

	private:
		NetworkingSystem() = default;


	private:
		IDVector	m_NetworkEntities;

	private:
		static NetworkingSystem s_Instance;
	};
}