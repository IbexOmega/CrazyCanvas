#pragma once

#include "ECS/ComponentOwner.h"
#include "ECS/System.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Math/Math.h"
#include "Physics/PhysX/ErrorCallback.h"
#include "Physics/PhysX/PhysX.h"

#include <variant>

namespace LambdaEngine
{
	using namespace physx;

	struct MeshComponent;
	struct PositionComponent;
	struct ScaleComponent;
	struct RotationComponent;
	struct VelocityComponent;

	enum class EShapeType
	{
		SIMULATION,	// A simulation shape is a regular physics object with collision detection and handling
		TRIGGER		// A trigger shape does not take part in the simulation, it only generates overlap reports
	};

	enum class ECollisionDetection
	{
		// Discrete collision detection is the default method. It is cheaper than continuous detection, but some collisions may be missed.
		DISCRETE,
		CONTINUOUS
	};

	typedef uint32 CollisionGroup;
	enum FCollisionGroup : CollisionGroup
	{
		COLLISION_GROUP_NONE	= 0,
		COLLISION_GROUP_STATIC	= (1 << 0),
		COLLISION_GROUP_DYNAMIC	= (1 << 1),

		COLLISION_GROUP_LAST	= COLLISION_GROUP_DYNAMIC,
	};

	// EntityCollisionInfo contains information on a colliding entity.
	struct EntityCollisionInfo
	{
		Entity Entity;
		glm::vec3 Position;
		glm::vec3 Direction;
	};

	typedef std::function<void(const EntityCollisionInfo& collisionInfo0, const EntityCollisionInfo& collisionInfo1)> CollisionCallback;
	typedef std::function<void(Entity entity0, Entity entity1)> TriggerCallback;

	// ActorUserData is stored in each PxActor::userData. It is used eg. when colliding.
	struct ActorUserData
	{
		Entity Entity;
		std::variant<CollisionCallback, TriggerCallback> CallbackFunction;
	};

	// CollisionCreateInfo contains information required to create a collision component
	struct CollisionCreateInfo
	{
		Entity Entity;
		const PositionComponent& Position;
		const ScaleComponent& Scale;
		const RotationComponent& Rotation;
		const MeshComponent& Mesh;
		EShapeType ShapeType;
		ECollisionDetection DetectionMethod = ECollisionDetection::DISCRETE;
		CollisionGroup CollisionGroup;				// The category of the object
		uint32 CollisionMask;						// Includes the masks of the groups this object collides with
		std::variant<CollisionCallback, TriggerCallback> CallbackFunction;	// Optional
	};

	struct DynamicCollisionCreateInfo : CollisionCreateInfo
	{
		const VelocityComponent& Velocity;	// Initial velocity
	};

	// CharacterColliderInfo contains information required to create a character collider
	struct CharacterColliderCreateInfo
	{
		Entity Entity;
		const PositionComponent& Position;
		const RotationComponent& Rotation;
		CollisionGroup CollisionGroup;		// The category of the object
		uint32 CollisionMask;				// Includes the masks of the groups this object collides with
	};

	class PhysicsSystem : public System, public ComponentOwner, public PxSimulationEventCallback
	{
	public:
		PhysicsSystem();
		~PhysicsSystem();

		bool Init();

		void Tick(Timestamp deltaTime) override final;

		/* Static collision actors */
		StaticCollisionComponent CreateStaticCollisionSphere(const CollisionCreateInfo& collisionInfo);
		StaticCollisionComponent CreateStaticCollisionBox(const CollisionCreateInfo& collisionInfo);
		StaticCollisionComponent CreateStaticCollisionCapsule(const CollisionCreateInfo& collisionInfo);
		StaticCollisionComponent CreateStaticCollisionMesh(const CollisionCreateInfo& collisionInfo);

		/* Dynamic collision actors */
		DynamicCollisionComponent CreateDynamicCollisionSphere(const DynamicCollisionCreateInfo& collisionInfo);
		DynamicCollisionComponent CreateDynamicCollisionBox(const DynamicCollisionCreateInfo& collisionInfo);
		DynamicCollisionComponent CreateDynamicCollisionCapsule(const DynamicCollisionCreateInfo& collisionInfo);
		DynamicCollisionComponent CreateDynamicCollisionMesh(const DynamicCollisionCreateInfo& collisionInfo);

		/* Character controllers */
		// CreateCharacterCapsule creates a character collider capsule. Total height is height + radius * 2 (+ contactOffset * 2)
		CharacterColliderComponent CreateCharacterCapsule(const CharacterColliderCreateInfo& characterColliderInfo, float height, float radius);
		// CreateCharacterBox creates a character collider box
		CharacterColliderComponent CreateCharacterBox(const CharacterColliderCreateInfo& characterColliderInfo, const glm::vec3& halfExtents);

		PxScene* GetScene() { return m_pScene; }

		/* Implement PxSimulationEventCallback */
		void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pPairs, PxU32 nbPairs) override final;
		void onTrigger(PxTriggerPair* pTriggerPairs, PxU32 nbPairs) override final;
		void onConstraintBreak(PxConstraintInfo*, PxU32) override final {}
		void onWake(PxActor**, PxU32 ) override final {}
		void onSleep(PxActor**, PxU32 ) override final {}
		void onAdvance(const PxRigidBody*const*, const PxTransform*, const PxU32) override final {}

		static PhysicsSystem* GetInstance() { return &s_Instance; }

	private:
		PxShape* CreateCollisionSphere(const CollisionCreateInfo& collisionInfo) const;
		PxShape* CreateCollisionBox(const CollisionCreateInfo& collisionInfo) const;
		// CreateCollisionCapsule creates a sphere if no capsule can be made
		PxShape* CreateCollisionCapsule(const CollisionCreateInfo& collisionInfo) const;
		PxShape* CreateCollisionTriangleMesh(const CollisionCreateInfo& collisionInfo) const;

		PxTransform CreatePxTransform(const glm::vec3& position, const glm::quat& rotation) const;

		void StaticCollisionDestructor(StaticCollisionComponent& collisionComponent);
		void DynamicCollisionDestructor(DynamicCollisionComponent& collisionComponent);
		void CharacterColliderDestructor(CharacterColliderComponent& characterColliderComponent);
		void ReleaseActor(PxRigidActor* pActor);

		void OnStaticCollisionAdded(Entity entity);
		void OnDynamicCollisionAdded(Entity entity);
		void OnStaticCollisionRemoval(Entity entity);
		void OnDynamicCollisionRemoval(Entity entity);
		void OnCharacterColliderRemoval(Entity entity);

		StaticCollisionComponent FinalizeStaticCollisionActor(const CollisionCreateInfo& collisionInfo, PxShape* pShape, const glm::quat& additionalRotation = glm::identity<glm::quat>());
		DynamicCollisionComponent FinalizeDynamicCollisionActor(const DynamicCollisionCreateInfo& collisionInfo, PxShape* pShape, const glm::quat& additionalRotation = glm::identity<glm::quat>());
		CharacterColliderComponent FinalizeCharacterController(const CharacterColliderCreateInfo& characterColliderInfo, PxControllerDesc& controllerDesc);
		void FinalizeCollisionActor(const CollisionCreateInfo& collisionInfo, PxRigidActor* pActor, PxShape* pShape);

		void TriggerCallbacks(const std::array<PxRigidActor*, 2>& actors) const;
		void CollisionCallbacks(const std::array<PxRigidActor*, 2>& actors, const TArray<PxContactPairPoint>& contactPoints) const;

	private:
		static PhysicsSystem s_Instance;

	private:
		IDVector m_StaticCollisionEntities;
		IDVector m_DynamicCollisionEntities;
		IDVector m_CharacterCollisionEntities;

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
