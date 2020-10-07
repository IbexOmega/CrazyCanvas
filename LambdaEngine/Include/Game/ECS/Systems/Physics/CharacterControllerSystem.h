#pragma once

#include "ECS/System.h"

#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

#include "ECS/ComponentOwner.h"

namespace LambdaEngine
{
	using namespace physx;

	class CharacterControllerSystem : public System, public ComponentOwner
	{
	public:
		CharacterControllerSystem();
		~CharacterControllerSystem();

		bool Init();

		void Tick(Timestamp deltaTime) override final;

		void FixedTickMainThread(Timestamp deltaTime);

	private:
		void TickCharacterControllers(float32 dt);
		void OnCharacterColliderRemoval(Entity entity);
		void CharacterColliderDestructor(CharacterColliderComponent& characterColliderComponent);

	public:
		static CharacterControllerSystem* GetInstance() { return s_pInstance; }
		static void TickCharacterController(float32 dt, Entity entity, ComponentArray<CharacterColliderComponent>* pCharacterColliders, ComponentArray<NetworkPositionComponent>* pNetPosComponents, ComponentArray<VelocityComponent>* pVelocityComponents);

	private:
		IDVector m_CharacterColliderEntities;

	private:
		static CharacterControllerSystem* s_pInstance;
	};
}
