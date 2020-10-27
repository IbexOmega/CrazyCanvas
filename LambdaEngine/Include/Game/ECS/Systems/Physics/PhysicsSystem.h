#pragma once

#include "ECS/ComponentOwner.h"
#include "ECS/System.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Math/Math.h"
#include "Physics/PhysX/ErrorCallback.h"
#include "Physics/PhysX/PhysX.h"

#include <variant>

namespace LambdaEngine
{
	using namespace physx;

	struct Mesh;
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

	enum class EGeometryType
	{
		SPHERE,
		BOX,
		CAPSULE, // A capsule's width is specified by its radius. Its height is radius * 2 + halfHeight * 2.
		MESH
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
	};

	// ShapeUserData is stored in each PxShape::userData. It is used eg. when colliding.
	struct ShapeUserData
	{
		std::variant<CollisionCallback, TriggerCallback> CallbackFunction;
		void* pUserData = nullptr;
	};

	struct ShapeCreateInfo
	{
		EShapeType ShapeType;
		EGeometryType GeometryType;

		union
		{
			//Sphere
			float32 Radius;

			//Box
			glm::vec3 HalfExtents;

			//Capsule
			struct
			{
				float32 Radius;
				float32 HalfHeight;
			};

			//Mesh
			Mesh* pMesh;
		} GeometryParams;

		CollisionGroup CollisionGroup;				// The category of the object
		uint32 CollisionMask;						// Includes the masks of the groups this object collides with

		std::variant<CollisionCallback, TriggerCallback> CallbackFunction;	// Optional

		void* pUserData = nullptr;		// Optional userdata, must not depend on a destructor being called on release
		uint32 UserDataSize = 0;
	};

	// CollisionCreateInfo contains information required to create a collision component
	struct CollisionCreateInfo
	{
		Entity Entity;
        ECollisionDetection DetectionMethod = ECollisionDetection::DISCRETE;
		const PositionComponent& Position;
		const ScaleComponent& Scale;
		const RotationComponent& Rotation;
		TArray<ShapeCreateInfo> Shapes;
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
		StaticCollisionComponent CreateStaticActor(const CollisionCreateInfo& collisionInfo);

		/* Dynamic collision actors */
		DynamicCollisionComponent CreateDynamicActor(const DynamicCollisionCreateInfo& collisionInfo);

		/* Character controllers */
		// CreateCharacterCapsule creates a character collider capsule. Total height is height + radius * 2 (+ contactOffset * 2)
		CharacterColliderComponent CreateCharacterCapsule(const CharacterColliderCreateInfo& characterColliderInfo, float32 height, float32 radius);
		// CreateCharacterBox creates a character collider box
		CharacterColliderComponent CreateCharacterBox(const CharacterColliderCreateInfo& characterColliderInfo, const glm::vec3& halfExtents);

		static float32 CalculateSphereRadius(const Mesh* pMesh);
		void CalculateCapsuleDimensions(Mesh* pMesh, float32& radius, float32& halfHeight);

		/* Implement PxSimulationEventCallback */
		void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pPairs, PxU32 nbPairs) override final;
		void onTrigger(PxTriggerPair* pTriggerPairs, PxU32 nbPairs) override final;
		void onConstraintBreak(PxConstraintInfo*, PxU32) override final {}
		void onWake(PxActor**, PxU32 ) override final {}
		void onSleep(PxActor**, PxU32 ) override final {}
		void onAdvance(const PxRigidBody*const*, const PxTransform*, const PxU32) override final {}

		PxScene* GetScene() { return m_pScene; }
		static PhysicsSystem* GetInstance() { return &s_Instance; }

	private:
		PxShape* CreateShape(const ShapeCreateInfo& shapeCreateInfo, const glm::vec3& scale) const;

		// CreateCollisionCapsule creates a sphere if no capsule can be made
		PxShape* CreateCollisionCapsule(float32 radius, float32 halfHeight) const;
		PxShape* CreateCollisionTriangleMesh(const Mesh* pMesh, const glm::vec3& scale) const;

		PxTransform CreatePxTransform(const glm::vec3& position, const glm::quat& rotation) const;

		static void StaticCollisionDestructor(StaticCollisionComponent& collisionComponent, Entity entity);
		static void DynamicCollisionDestructor(DynamicCollisionComponent& collisionComponent, Entity entity);
		static void CharacterColliderDestructor(CharacterColliderComponent& characterColliderComponent, Entity entity);
		static void ReleaseActor(PxRigidActor* pActor);

		void OnStaticCollisionAdded(Entity entity);
		void OnDynamicCollisionAdded(Entity entity);
		void OnStaticCollisionRemoval(Entity entity);
		void OnDynamicCollisionRemoval(Entity entity);
		void OnCharacterColliderRemoval(Entity entity);

		StaticCollisionComponent FinalizeStaticCollisionActor(const CollisionCreateInfo& collisionInfo, const glm::quat& additionalRotation = glm::identity<glm::quat>());
		DynamicCollisionComponent FinalizeDynamicCollisionActor(const DynamicCollisionCreateInfo& collisionInfo, const glm::quat& additionalRotation = glm::identity<glm::quat>());
		CharacterColliderComponent FinalizeCharacterController(const CharacterColliderCreateInfo& characterColliderInfo, PxControllerDesc& controllerDesc);
		void FinalizeCollisionActor(const CollisionCreateInfo& collisionInfo, PxRigidActor* pActor);

		void TriggerCallbacks(const std::array<PxRigidActor*, 2>& actors, const std::array<PxShape*, 2>& shapes) const;
		void CollisionCallbacks(const std::array<PxRigidActor*, 2>& actors, const std::array<PxShape*, 2>& shapes, const TArray<PxContactPairPoint>& contactPoints) const;

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
