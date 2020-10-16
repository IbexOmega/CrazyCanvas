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
		friend class ClientSystem;
		friend class ServerSystem;

	public:
		virtual ~CharacterControllerSystem();

	private:
		CharacterControllerSystem();

		bool Init();
		void Tick(Timestamp deltaTime) override final;

		void OnCharacterColliderRemoval(Entity entity);
		void CharacterColliderDestructor(CharacterColliderComponent& characterColliderComponent);

	public:
		static CharacterControllerSystem* GetInstance() { return s_pInstance; }

		static void TickForeignCharacterController(float32 dt,
			Entity entity,
			ComponentArray<CharacterColliderComponent>* pCharacterColliders,
			const ComponentArray<NetworkPositionComponent>* pNetPosComponents,
			ComponentArray<VelocityComponent>* pVelocityComponents);

		static void TickCharacterController(float32 dt,
			Entity entity,
			ComponentArray<CharacterColliderComponent>* pCharacterColliders,
			const ComponentArray<NetworkPositionComponent>* pNetPosComponents,
			ComponentArray<VelocityComponent>* pVelocityComponents);

	private:
		IDVector m_ForeignPlayerEntities;

	private:
		static CharacterControllerSystem* s_pInstance;
	};
}
