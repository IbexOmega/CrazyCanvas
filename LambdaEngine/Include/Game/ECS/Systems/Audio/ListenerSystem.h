#pragma once
#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Audio/ListenerComponent.h"

namespace LambdaEngine
{
	class ListenerSystem : public System
	{
	public:
		DECL_UNIQUE_CLASS(ListenerSystem);
		~ListenerSystem() = default;

		bool Init();

		void Tick(Timestamp deltaTime) override;

	public:
		static ListenerSystem& GetInstance() { return s_Instance; }

	private:
		ListenerSystem() = default;

	private:
		IDVector	m_ListenerEntities;

	private:
		static ListenerSystem s_Instance;
	};
}