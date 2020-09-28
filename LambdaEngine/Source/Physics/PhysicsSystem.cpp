#include "Physics/PhysicsSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"

#define PX_RELEASE(x) if(x)	{ x->release(); x = nullptr; }

#define GRAVITATIONAL_ACCELERATION 9.82f

namespace LambdaEngine
{
	PhysicsSystem PhysicsSystem::s_Instance;

	PhysicsSystem::PhysicsSystem()
		:m_pFoundation(nullptr),
		m_pPhysics(nullptr),
		m_pDispatcher(nullptr),
		m_pScene(nullptr)
	{}

	PhysicsSystem::~PhysicsSystem()
	{
		PX_RELEASE(m_pDispatcher);
		PX_RELEASE(m_pScene);
		PX_RELEASE(m_pPhysics);
		PX_RELEASE(m_pFoundation);
	}

	bool PhysicsSystem::Init()
	{
		{
			SystemRegistration systemReg = {};
			systemReg.Phase = 1;

			RegisterSystem(systemReg);
		}

		// PhysX setup
		m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_Allocator, m_ErrorCallback);
		if (!m_pFoundation)
		{
			LOG_ERROR("PhysX foundation creation failed");
			return false;
		}

		m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, PxTolerancesScale(), false, nullptr);
		if (!m_pPhysics)
		{
			LOG_ERROR("PhysX core creation failed");
			return false;
		}

		m_pDispatcher = PxDefaultCpuDispatcherCreate(2);
		if (!m_pDispatcher)
		{
			LOG_ERROR("PhysX CPU dispatcher creation failed");
			return false;
		}

		const PxVec3 gravity = {
			g_DefaultUp.x,
			-g_DefaultUp.y * 9.81f,
			g_DefaultUp.z
		};

		PxSceneDesc sceneDesc(m_pPhysics->getTolerancesScale());
		sceneDesc.gravity		= gravity;
		sceneDesc.cpuDispatcher	= m_pDispatcher;
		sceneDesc.filterShader	= PxDefaultSimulationFilterShader;
		m_pScene = m_pPhysics->createScene(sceneDesc);
		if (!m_pScene)
		{
			LOG_ERROR("PhysX scene creation failed");
			return false;
		}

		return true;
	}

	void PhysicsSystem::Tick(Timestamp deltaTime)
	{
		m_pScene->simulate((float)deltaTime.AsSeconds());
		m_pScene->fetchResults(true);
	}
}
