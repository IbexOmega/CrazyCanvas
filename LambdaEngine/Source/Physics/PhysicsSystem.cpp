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
			// Subscribe to entities to register a destructor for collision components
			auto onCollisionRemoved = std::bind(&PhysicsSystem::OnCollisionRemoved, this, std::placeholders::_1);

			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{NDA, CollisionComponent::Type()}}, &m_MeshEntities, nullptr, onCollisionRemoved},
			};
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
			if (m_pVisDbg->connect(*transport,PxPvdInstrumentationFlag::eALL))
			{
				LOG_INFO("Connected to PhysX debug visualizer at %s", PVD_HOST);
			}
			else
			{
				LOG_WARNING("Failed to connect to PhysX debug visualizer at %s", PVD_HOST);
			}
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

	CollisionComponent& PhysicsSystem::CreateCollisionSphere(const CollisionCreateInfo& collisionCreateInfo)
	{
		const Mesh* pMesh = ResourceManager::GetMesh(collisionCreateInfo.Mesh.MeshGUID);
		const TArray<Vertex>& vertices = pMesh->Vertices;

		float squareRadius = 0.0f;

		for (const Vertex& vertex : vertices)
		{
			squareRadius = std::max(squareRadius, glm::length2(vertex.Position));
		}

		PxShape* pSphereShape = m_pPhysics->createShape(PxSphereGeometry(std::sqrtf(squareRadius)), *m_pMaterial);
		return FinalizeCollisionComponent(collisionCreateInfo, pSphereShape);
	}

	CollisionComponent& PhysicsSystem::CreateCollisionBox(const CollisionCreateInfo& collisionCreateInfo)
	{
		const Mesh* pMesh = ResourceManager::GetMesh(collisionCreateInfo.Mesh.MeshGUID);
		const glm::vec3& halfExtent = pMesh->BoundingBox.HalfExtent;
		const PxVec3 halfExtentPX(halfExtent.x, halfExtent.y, halfExtent.z);

		PxShape* pBoxShape = m_pPhysics->createShape(PxBoxGeometry(halfExtentPX), *m_pMaterial);
		return FinalizeCollisionComponent(collisionCreateInfo, pBoxShape);
	}

	CollisionComponent& PhysicsSystem::CreateCollisionCapsule(const CollisionCreateInfo& collisionCreateInfo)
	{
		/*	A PhysX capsule's height extends along the x-axis. To make the capsule stand upright,
			it is rotated around the z-axis. */
		const Mesh* pMesh = ResourceManager::GetMesh(collisionCreateInfo.Mesh.MeshGUID);
		const TArray<Vertex>& vertices = pMesh->Vertices;

		// The radius in the XZ plane (horizontal)
		float squareRadiusXZ = 0.0f;
		float halfHeight = 0.0f;

		for (const Vertex& vertex : vertices)
		{
			const glm::vec3& position = vertex.Position;
			squareRadiusXZ = std::max(squareRadiusXZ, glm::length2(glm::vec3(position.x, 0.0f, position.z)));
			halfHeight = std::max(halfHeight, std::abs(position.y));
		}

		const float capsuleRadius = std::sqrtf(squareRadiusXZ);
		halfHeight = std::max(0.0f, halfHeight - capsuleRadius);

		PxShape* pShape = nullptr;
		if (halfHeight != 0.0f)
		{
			pShape = m_pPhysics->createShape(PxCapsuleGeometry(capsuleRadius, halfHeight), *m_pMaterial);
		}
		else
		{
			pShape = m_pPhysics->createShape(PxSphereGeometry(capsuleRadius), *m_pMaterial);
		}

		const PxQuat uprightRotation = PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f));
		return FinalizeCollisionComponent(collisionCreateInfo, pShape, uprightRotation);
	}

	void PhysicsSystem::RemoveCollisionActor(Entity entity)
	{
		OnCollisionRemoved(entity);
		ECSCore::GetInstance()->RemoveComponent<CollisionComponent>(entity);
	}

	void PhysicsSystem::OnCollisionRemoved(Entity entity)
	{
		PxActor* pActor = m_Actors.IndexID(entity);
		m_Actors.Pop(entity);
		m_pScene->removeActor(*pActor);
	}

	CollisionComponent& PhysicsSystem::FinalizeCollisionComponent(const CollisionCreateInfo& collisionCreateInfo, PxShape* pShape, const PxQuat& additionalRotation)
	{
		const glm::vec3& position = collisionCreateInfo.Position.Position;
		const glm::quat& rotation = collisionCreateInfo.Rotation.Quaternion;

		const PxVec3 positionPX = { position.x, position.y, position.z };
		const PxQuat rotationPX = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w) * additionalRotation;
		const PxTransform transformPX(positionPX, rotationPX);

		PxRigidStatic* pBody = m_pPhysics->createRigidStatic(transformPX);
		pBody->attachShape(*pShape);

		m_pScene->addActor(*pBody);
		m_Actors.PushBack(pBody, collisionCreateInfo.Entity);

		pShape->release();

		CollisionComponent collisionComponent = {};
		return ECSCore::GetInstance()->AddComponent<CollisionComponent>(collisionCreateInfo.Entity, collisionComponent);
	}
}
