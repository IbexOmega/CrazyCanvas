#pragma once

#include "ECS/System.h"

#include <btBulletDynamicsCommon.h>

namespace LambdaEngine
{
	class PhysicsSystem : public System
	{
	public:
		PhysicsSystem();
		~PhysicsSystem();

		bool Init();

		void Tick(Timestamp deltaTime) override final;

		static PhysicsSystem* GetInstance() { return &s_Instance; }

	private:
		static PhysicsSystem s_Instance;

	private:
		btDbvtBroadphase m_BroadphaseInterface;
		btDefaultCollisionConfiguration m_CollisionConfiguration;
		btCollisionDispatcher* m_pCollisionDispatcher;
		btSequentialImpulseConstraintSolver m_ConstraintSolver;
		btDiscreteDynamicsWorld* m_pDynamicsWorld;
	};
}
