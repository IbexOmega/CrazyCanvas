#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

namespace LambdaEngine
{
	class EntityFactorySystem : public System
	{
	public:
		DECL_UNIQUE_CLASS(EntityFactorySystem);
		~EntityFactorySystem() = default;

		bool Init();

		void Tick(Timestamp deltaTime) override;

	public:
		static EntityFactorySystem& GetInstance() { return s_Instance; }

	private:
		EntityFactorySystem() = default;


	private:
		IDVector	m_NetworkEntities;

	private:
		static EntityFactorySystem s_Instance;
	};
}