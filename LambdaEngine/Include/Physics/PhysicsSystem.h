#pragma once

#include "ECS/ComponentOwner.h"
#include "ECS/System.h"
#include "Game/ECS/Components/Physics/Collision.h"

#undef realloc
#undef free
#include <PxPhysicsAPI.h>
#include <foundation/PxErrorCallback.h>

namespace LambdaEngine
{
	using namespace physx;

	struct PositionComponent;
	struct ScaleComponent;
	struct RotationComponent;
	struct MeshComponent;

	enum FCollisionGroup : int16
	{
		COLLISION_GROUP_NONE	= 0,
		COLLISION_GROUP_STATIC	= 1
	};

	// CollisionCreateInfo contains information required to create a collision component
	struct CollisionCreateInfo
	{
		Entity Entity;
		const PositionComponent& Position;
		const ScaleComponent& Scale;
		const RotationComponent& Rotation;
		const MeshComponent& Mesh;
		FCollisionGroup CollisionGroup;	// The category of the object
		FCollisionGroup CollisionMask;	// Includes the masks of the groups this object collides with
	};

	class PhysicsSystem : public System, public ComponentOwner
	{
	public:
		PhysicsSystem();
		~PhysicsSystem();

		bool Init();

		void Tick(Timestamp deltaTime) override final;

		void CreateCollisionComponent(const CollisionCreateInfo& collisionCreateInfo);

		static PhysicsSystem* GetInstance() { return &s_Instance; }

	private:
		void CollisionComponentDestructor(CollisionComponent& collisionComponent);

	private:
		static PhysicsSystem s_Instance;

	private:
		IDVector m_MeshEntities;

		PxDefaultAllocator		m_Allocator;
		PxDefaultErrorCallback	m_ErrorCallback;

		PxFoundation*	m_pFoundation;
		PxPhysics*		m_pPhysics;
		PxPvd*			m_pVisDbg; // Visual debugger

		PxDefaultCpuDispatcher*	m_pDispatcher;
		PxScene*				m_pScene;

		PxMaterial* m_pMaterial;
	};
}
