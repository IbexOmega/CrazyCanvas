#include "Physics/PhysicsSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"

#define GRAVITATIONAL_ACCELERATION 9.82f

namespace LambdaEngine
{
	PhysicsSystem PhysicsSystem::s_Instance;

	PhysicsSystem::PhysicsSystem()
		:m_pCollisionDispatcher(nullptr),
		m_pDynamicsWorld(nullptr)
	{
		SystemRegistration systemReg = {};
		systemReg.Phase = 1;

		RegisterSystem(systemReg);
	}

	PhysicsSystem::~PhysicsSystem()
	{
		SAFEDELETE(m_pCollisionDispatcher);
		SAFEDELETE(m_pDynamicsWorld);
	}

	bool PhysicsSystem::Init()
	{
		m_pCollisionDispatcher = DBG_NEW btCollisionDispatcher(&m_CollisionConfiguration);
		m_pDynamicsWorld = new btDiscreteDynamicsWorld(
			m_pCollisionDispatcher,
			&m_BroadphaseInterface,
			&m_ConstraintSolver,
			&m_CollisionConfiguration
		);

		const btVector3 gravity = {
			g_DefaultUp.x,
			-g_DefaultUp.y * GRAVITATIONAL_ACCELERATION,
			g_DefaultUp.z
		};

		m_pDynamicsWorld->setGravity(gravity);

		return m_pCollisionDispatcher && m_pDynamicsWorld;
	}

	void PhysicsSystem::Tick(Timestamp deltaTime)
	{
		const btScalar deltaBT = (float)deltaTime.AsSeconds();
		m_pDynamicsWorld->stepSimulation(deltaBT);
	}
}
