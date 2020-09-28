#include "Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Resources/ResourceManager.h"

#define PX_RELEASE(x) if(x)	{ x->release(); x = nullptr; }
#define PVD_HOST "127.0.0.1"	// The IP address to stream debug visualization data to

namespace LambdaEngine
{
	PhysicsSystem PhysicsSystem::s_Instance;

	PhysicsSystem::PhysicsSystem()
		:m_pFoundation(nullptr),
		m_pPhysics(nullptr),
		m_pVisDbg(nullptr),
		m_pDispatcher(nullptr),
		m_pScene(nullptr),
		m_pMaterial(nullptr)
	{}

	PhysicsSystem::~PhysicsSystem()
	{
		PX_RELEASE(m_pMaterial);
		PX_RELEASE(m_pDispatcher);
		PX_RELEASE(m_pScene);
		PX_RELEASE(m_pPhysics);

		if(m_pVisDbg)
		{
			PxPvdTransport* pTransport = m_pVisDbg->getTransport();
			m_pVisDbg->release();
			m_pVisDbg = nullptr;
			PX_RELEASE(pTransport);
		}

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

	#ifdef LAMBDA_DEBUG
		if (EngineConfig::GetBoolProperty("StreamPhysx"))
		{
			m_pVisDbg = PxCreatePvd(*m_pFoundation);
			PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
			m_pVisDbg->connect(*transport,PxPvdInstrumentationFlag::eALL);
		}
	#endif // LAMBDA_DEBUG

		m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, PxTolerancesScale(), false, m_pVisDbg);
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

		m_pMaterial = m_pPhysics->createMaterial(0.5f, 0.5f, 0.6f);
		return m_pMaterial;
	}

	void PhysicsSystem::Tick(Timestamp deltaTime)
	{
		m_pScene->simulate((float)deltaTime.AsSeconds());
		m_pScene->fetchResults(true);
	}

	void PhysicsSystem::CreateCollisionComponent(const CollisionCreateInfo& collisionCreateInfo)
	{
		const glm::vec3& position = collisionCreateInfo.Position.Position;
		const glm::quat& rotation = collisionCreateInfo.Rotation.Quaternion;

		const Mesh* pMesh = ResourceManager::GetMesh(collisionCreateInfo.Mesh.MeshGUID);
		const glm::vec3& halfExtent = pMesh->BoundingBox.HalfExtent;

		const PxVec3 positionPX = { position.x, position.y, position.z };
		const PxQuat rotationPX = { rotation.x, rotation.y, rotation.z, rotation.w };
		const PxTransform transformPX(positionPX, rotationPX);
		PxRigidStatic* pBody = m_pPhysics->createRigidStatic(transformPX);

		PxShape* pBoxShape = m_pPhysics->createShape(PxBoxGeometry(halfExtent.x, halfExtent.y, halfExtent.z), *m_pMaterial);
		pBody->attachShape(*pBoxShape);

		m_pScene->addActor(*pBody);

		pBoxShape->release();

		CollisionComponent collisionComponent = {};
		ECSCore::GetInstance()->AddComponent<CollisionComponent>(collisionCreateInfo.Entity, collisionComponent);
	}
}
