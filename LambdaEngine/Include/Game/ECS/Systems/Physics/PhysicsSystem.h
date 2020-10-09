#pragma once

#include "ECS/ComponentOwner.h"
#include "ECS/System.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Physics/PhysXErrorCallback.h"

#undef realloc
#undef free
#include <PxPhysicsAPI.h>
#include <foundation/PxErrorCallback.h>

namespace LambdaEngine
{
	using namespace physx;

	struct FPSControllerComponent;
	struct MeshComponent;
	struct PositionComponent;
	struct RotationComponent;
	struct ScaleComponent;

	enum FCollisionGroup : uint32
	{
		COLLISION_GROUP_NONE	= 0,
		COLLISION_GROUP_STATIC	= (1 << 0),
		COLLISION_GROUP_PLAYER	= (1 << 1),
	};

	// CollisionCreateInfo contains information required to create a collision component
	struct StaticCollisionInfo
	{
		Entity Entity;
		const PositionComponent& Position;
		const ScaleComponent& Scale;
		const RotationComponent& Rotation;
		const MeshComponent& Mesh;
		FCollisionGroup CollisionGroup;	// The category of the object
		uint32 CollisionMask;	// Includes the masks of the groups this object collides with
	};

	// CharacterColliderInfo contains information required to create a character collider
	struct CharacterColliderInfo
	{
		Entity Entity;
		const PositionComponent& Position;
		const RotationComponent& Rotation;
		FCollisionGroup CollisionGroup;	// The category of the object
		uint32 CollisionMask;	// Includes the masks of the groups this object collides with
	};

	class PhysicsSystem : public System, public ComponentOwner
	{
	public:
		PhysicsSystem();
		~PhysicsSystem();

		bool Init();

		void Tick(Timestamp deltaTime) override final;

		/* Static collision actors */
		void CreateCollisionSphere(const StaticCollisionInfo& staticCollisionInfo);
		void CreateCollisionBox(const StaticCollisionInfo& staticCollisionInfo);
		// CreateCollisionCapsule creates a sphere if no capsule can be made
		void CreateCollisionCapsule(const StaticCollisionInfo& staticCollisionInfo);
		void CreateCollisionTriangleMesh(const StaticCollisionInfo& staticCollisionInfo);

		/* Character controllers */
		// CreateCharacterCapsule creates a character collider capsule. Total height is height + radius * 2
		void CreateCharacterCapsule(const CharacterColliderInfo& characterColliderInfo, float height, float radius, CharacterColliderComponent& characterColliderComp);
		// CreateCharacterBox creates a character collider box
		void CreateCharacterBox(const CharacterColliderInfo& characterColliderInfo, const glm::vec3& halfExtents, CharacterColliderComponent& characterColliderComp);

		PxScene* GetScene() { return m_pScene; }

		static PhysicsSystem* GetInstance() { return &s_Instance; }

	private:
		void StaticCollisionDestructor(StaticCollisionComponent& collisionComponent);
		void OnStaticCollisionRemoval(Entity entity);

		static glm::vec3 GetCharacterTranslation(float32 dt, const glm::vec3& forward, const glm::vec3& right, const FPSControllerComponent& FPSComp);

		void FinalizeCollisionComponent(const StaticCollisionInfo& collisionCreateInfo, PxShape* pShape, const PxQuat& additionalRotation = PxQuat(PxIDENTITY::PxIdentity));
		void FinalizeCharacterController(const CharacterColliderInfo& characterColliderInfo, PxControllerDesc& controllerDesc, CharacterColliderComponent& characterColliderComp);

	private:
		static PhysicsSystem s_Instance;

	private:
		IDVector m_StaticCollisionEntities;

		PxDefaultAllocator		m_Allocator;
		PhysXErrorCallback		m_ErrorCallback;

		PxFoundation*			m_pFoundation;
		PxPhysics*				m_pPhysics;
		PxCooking*				m_pCooking;
		PxControllerManager*	m_pControllerManager;
		PxPvd*					m_pVisDbg; // Visual debugger

		PxDefaultCpuDispatcher*	m_pDispatcher;
		PxScene*				m_pScene;

		PxMaterial* m_pMaterial;
	};
}
