#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "Networking/API/IClient.h"

namespace LambdaEngine
{
	class PlayerMovementSystem : public System
	{
	public:
		DECL_UNIQUE_CLASS(PlayerMovementSystem);
		~PlayerMovementSystem() = default;

		bool Init();

		void FixedTick(Timestamp deltaTime);
		void Tick(Timestamp deltaTime) override;

		void Move(Entity entity, Timestamp deltaTime, int8 deltaForward, int8 deltaLeft);

	public:
		static PlayerMovementSystem& GetInstance() { return s_Instance; }

	private:
		PlayerMovementSystem() = default;

	private:
		IDVector	m_ControllableEntities;

	private:
		static PlayerMovementSystem s_Instance;
	};
}