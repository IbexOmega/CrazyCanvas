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
		m_pCooking(nullptr),
		m_pVisDbg(nullptr),
		m_pDispatcher(nullptr),
		m_pScene(nullptr),
		m_pMaterial(nullptr)
	{}

	PhysicsSystem::~PhysicsSystem()
	{
		PX_RELEASE(m_pMaterial);
		PX_RELEASE(m_pCooking);
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
			auto onCollisionRemoved = std::bind(&PhysicsSystem::OnCollisionEntityRemoved, this, std::placeholders::_1);

			TransformComponents transformComponents;
			transformComponents.Position.Permissions	= RW;
			transformComponents.Scale.Permissions		= NDA;
			transformComponents.Rotation.Permissions	= RW;

			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{NDA, CollisionComponent::Type()}}, {&transformComponents}, &m_CollisionEntities, nullptr, onCollisionRemoved},
			};
			systemReg.Phase = 1;

			RegisterSystem(systemReg);
			SetComponentOwner<CollisionComponent>({ std::bind(&PhysicsSystem::CollisionComponentDestructor, this, std::placeholders::_1) });
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

		const PxTolerancesScale tolerancesScale;
		m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, tolerancesScale, false, m_pVisDbg);
		if (!m_pPhysics)
		{
			LOG_ERROR("PhysX core creation failed");
			return false;
		}

		m_pCooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_pFoundation, PxCookingParams(tolerancesScale));
		if (!m_pCooking)
		{
			LOG_ERROR("PhysX cooking creation failed");
			return false;
		}

		// The 'Geometry' section in the PhysX user guide explains these parameters
		PxCookingParams cookingParams(tolerancesScale);
		cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
		cookingParams.suppressTriangleMeshRemapTable = true;
		cookingParams.meshPreprocessParams = PxMeshPreprocessingFlag::eWELD_VERTICES;
		m_pCooking->setParams(cookingParams);

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

	void PhysicsSystem::CreateCollisionSphere(const CollisionCreateInfo& collisionCreateInfo)
	{
		const Mesh* pMesh = ResourceManager::GetMesh(collisionCreateInfo.Mesh.MeshGUID);
		const TArray<Vertex>& vertices = pMesh->Vertices;

		float squareRadius = 0.0f;

		for (const Vertex& vertex : vertices)
			squareRadius = std::max(squareRadius, glm::length2(vertex.Position));

		const glm::vec3& scale = collisionCreateInfo.Scale.Scale;
		const float scaleScalar = std::max(scale.x, std::max(scale.y, scale.z));
		const float radius = std::sqrtf(squareRadius) * scaleScalar;

		PxShape* pSphereShape = m_pPhysics->createShape(PxSphereGeometry(radius), *m_pMaterial);
		FinalizeCollisionComponent(collisionCreateInfo, pSphereShape);
	}

	void PhysicsSystem::CreateCollisionBox(const CollisionCreateInfo& collisionCreateInfo)
	{
		const Mesh* pMesh = ResourceManager::GetMesh(collisionCreateInfo.Mesh.MeshGUID);
		const glm::vec3& halfExtent = pMesh->BoundingBox.HalfExtent * collisionCreateInfo.Scale.Scale;
		const PxVec3 halfExtentPX(halfExtent.x, halfExtent.y, halfExtent.z);

		PxShape* pBoxShape = m_pPhysics->createShape(PxBoxGeometry(halfExtentPX), *m_pMaterial);
		FinalizeCollisionComponent(collisionCreateInfo, pBoxShape);
	}

	void PhysicsSystem::CreateCollisionCapsule(const CollisionCreateInfo& collisionCreateInfo)
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
		PxQuat uprightRotation = PxQuat(PxIdentity);
		if (halfHeight != 0.0f)
		{
			pShape = m_pPhysics->createShape(PxCapsuleGeometry(capsuleRadius, halfHeight), *m_pMaterial);
			uprightRotation = PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f));
		}
		else
		{
			pShape = m_pPhysics->createShape(PxSphereGeometry(capsuleRadius), *m_pMaterial);
		}

		FinalizeCollisionComponent(collisionCreateInfo, pShape, uprightRotation);
	}

	void PhysicsSystem::CreateCollisionTriangleMesh(const CollisionCreateInfo& collisionCreateInfo)
	{
		/* PhysX is capable of 'cooking' meshes; generating an optimized collision mesh from triangle data */
		const Mesh* pMesh = ResourceManager::GetMesh(collisionCreateInfo.Mesh.MeshGUID);
		const TArray<Vertex>& vertices = pMesh->Vertices;

		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count	= vertices.GetSize();
		meshDesc.points.stride	= sizeof(Vertex);
		meshDesc.points.data	= vertices.GetData();

		const TArray<uint32>& indices = pMesh->Indices;
		// 'Triangles' refer to triangle indices
		meshDesc.triangles.count	= indices.GetSize() / 3;
		meshDesc.triangles.stride	= 3 * sizeof(uint32);
		meshDesc.triangles.data		= indices.GetData();

		PxDefaultMemoryOutputStream writeBuffer;
		PxTriangleMeshCookingResult::Enum result;
		bool status = m_pCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
		if (!status)
		{
			LOG_WARNING("Failed to cook mesh with %d vertices", vertices.GetSize());
			return;
		}

		PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		PxTriangleMesh* pTriangleMesh = m_pPhysics->createTriangleMesh(readBuffer);

		// Create a geometry instance of the mesh and scale it
		const glm::vec3 scale = collisionCreateInfo.Scale.Scale;

		PxTriangleMeshGeometry triangleMeshGeometry(pTriangleMesh, PxMeshScale({ scale.x, scale.y, scale.z }));
		PxShape* pShape = m_pPhysics->createShape(triangleMeshGeometry, *m_pMaterial);

		FinalizeCollisionComponent(collisionCreateInfo, pShape);
	}

	void PhysicsSystem::CollisionComponentDestructor(CollisionComponent& collisionComponent)
	{
		m_pScene->removeActor(*collisionComponent.pActor);
		PX_RELEASE(collisionComponent.pActor);
	}

	void PhysicsSystem::OnCollisionEntityRemoved(Entity entity)
	{
		// Remove the actor from the scene
		CollisionComponent& collisionComponent = ECSCore::GetInstance()->GetComponent<CollisionComponent>(entity);
		PxActor* pActor = collisionComponent.pActor;
		if (pActor)
		{
			m_pScene->removeActor(*pActor);
		}
	}

	void PhysicsSystem::FinalizeCollisionComponent(const CollisionCreateInfo& collisionCreateInfo, PxShape* pShape, const PxQuat& additionalRotation)
	{
		const glm::vec3& position = collisionCreateInfo.Position.Position;
		const glm::quat& rotation = collisionCreateInfo.Rotation.Quaternion;

		const PxVec3 positionPX = { position.x, position.y, position.z };
		const PxQuat rotationPX = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w) * additionalRotation;
		const PxTransform transformPX(positionPX, rotationPX);

		PxRigidStatic* pBody = m_pPhysics->createRigidStatic(transformPX);
		pBody->attachShape(*pShape);

		m_pScene->addActor(*pBody);
		pShape->release();

		CollisionComponent collisionComponent = { pBody };
		ECSCore::GetInstance()->AddComponent<CollisionComponent>(collisionCreateInfo.Entity, collisionComponent);
	}
}
