#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Input/API/InputActionSystem.h"
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
		m_pControllerManager(nullptr),
		m_pVisDbg(nullptr),
		m_pDispatcher(nullptr),
		m_pScene(nullptr),
		m_pMaterial(nullptr)
	{}

	PhysicsSystem::~PhysicsSystem()
	{
		PX_RELEASE(m_pMaterial);
		PX_RELEASE(m_pCooking);
		PX_RELEASE(m_pControllerManager);
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
			auto onStaticCollisionRemoval = std::bind(&PhysicsSystem::OnStaticCollisionRemoval, this, std::placeholders::_1);

			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					.pSubscriber = &m_StaticCollisionEntities,
					.ComponentAccesses =
					{
						{RW, StaticCollisionComponent::Type()}, {RW, PositionComponent::Type()}, {RW, RotationComponent::Type()}
					},
					.OnEntityRemoval = onStaticCollisionRemoval
				}
			};
			systemReg.Phase = 1;

			RegisterSystem(systemReg);
			SetComponentOwner<StaticCollisionComponent>({ std::bind(&PhysicsSystem::StaticCollisionDestructor, this, std::placeholders::_1) });
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
		cookingParams.midphaseDesc						= PxMeshMidPhase::eBVH34;
		cookingParams.suppressTriangleMeshRemapTable	= true;
		cookingParams.meshPreprocessParams				= PxMeshPreprocessingFlag::eWELD_VERTICES;
		cookingParams.meshWeldTolerance					= 0.1f;
		m_pCooking->setParams(cookingParams);

		m_pDispatcher = PxDefaultCpuDispatcherCreate(2);
		if (!m_pDispatcher)
		{
			LOG_ERROR("PhysX CPU dispatcher creation failed");
			return false;
		}

		const PxVec3 gravity = {
			g_DefaultUp.x,
			-g_DefaultUp.y * GRAVITATIONAL_ACCELERATION,
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

		m_pControllerManager = PxCreateControllerManager(*m_pScene);
		if (!m_pControllerManager)
		{
			LOG_ERROR("PhysX controller manager creation failed");
			return false;
		}

		m_pMaterial = m_pPhysics->createMaterial(0.5f, 0.5f, 0.6f);
		return m_pMaterial;
	}

	void PhysicsSystem::Tick(Timestamp deltaTime)
	{
		const float32 dt = (float32)deltaTime.AsSeconds();

		m_pScene->simulate(dt);
		m_pScene->fetchResults(true);
	}

	void PhysicsSystem::CreateCollisionSphere(const StaticCollisionInfo& staticCollisionInfo)
	{
		const Mesh* pMesh = ResourceManager::GetMesh(staticCollisionInfo.Mesh.MeshGUID);
		const TArray<Vertex>& vertices = pMesh->Vertices;

		float squareRadius = 0.0f;

		for (const Vertex& vertex : vertices)
		{
			squareRadius = std::max(squareRadius, glm::length2(vertex.Position));
		}

		const glm::vec3& scale = staticCollisionInfo.Scale.Scale;
		const float scaleScalar = std::max(scale.x, std::max(scale.y, scale.z));
		const float radius = std::sqrtf(squareRadius) * scaleScalar;

		PxShape* pSphereShape = m_pPhysics->createShape(PxSphereGeometry(radius), *m_pMaterial);
		FinalizeCollisionComponent(staticCollisionInfo, pSphereShape);
	}

	void PhysicsSystem::CreateCollisionBox(const StaticCollisionInfo& staticCollisionInfo)
	{
		const Mesh* pMesh = ResourceManager::GetMesh(staticCollisionInfo.Mesh.MeshGUID);
		const glm::vec3& halfExtent = pMesh->BoundingBox.HalfExtent * staticCollisionInfo.Scale.Scale;
		const PxVec3 halfExtentPX(halfExtent.x, halfExtent.y, halfExtent.z);

		PxShape* pBoxShape = m_pPhysics->createShape(PxBoxGeometry(halfExtentPX), *m_pMaterial);
		FinalizeCollisionComponent(staticCollisionInfo, pBoxShape);
	}

	void PhysicsSystem::CreateCollisionCapsule(const StaticCollisionInfo& staticCollisionInfo)
	{
		/*	A PhysX capsule's height extends along the x-axis. To make the capsule stand upright,
			it is rotated around the z-axis. */
		const Mesh* pMesh = ResourceManager::GetMesh(staticCollisionInfo.Mesh.MeshGUID);
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

		FinalizeCollisionComponent(staticCollisionInfo, pShape, uprightRotation);
	}

	void PhysicsSystem::CreateCollisionTriangleMesh(const StaticCollisionInfo& staticCollisionInfo)
	{
		/* PhysX is capable of 'cooking' meshes; generating an optimized collision mesh from triangle data */
		const Mesh* pMesh = ResourceManager::GetMesh(staticCollisionInfo.Mesh.MeshGUID);
		const TArray<Vertex>& vertices = pMesh->Vertices;

		PxTriangleMeshDesc meshDesc;
		meshDesc.flags			= PxMeshFlag::eFLIPNORMALS;
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
		const glm::vec3 scale = staticCollisionInfo.Scale.Scale;

		PxTriangleMeshGeometry triangleMeshGeometry(pTriangleMesh, PxMeshScale({ scale.x, scale.y, scale.z }));
		PxShape* pShape = m_pPhysics->createShape(triangleMeshGeometry, *m_pMaterial);

		FinalizeCollisionComponent(staticCollisionInfo, pShape);
	}

	void PhysicsSystem::StaticCollisionDestructor(StaticCollisionComponent& collisionComponent)
	{
		PX_RELEASE(collisionComponent.pActor);
	}

	void PhysicsSystem::OnStaticCollisionRemoval(Entity entity)
	{
		// Remove the actor from the scene
		StaticCollisionComponent& collisionComponent = ECSCore::GetInstance()->GetComponent<StaticCollisionComponent>(entity);
		PxActor* pActor = collisionComponent.pActor;
		if (pActor)
		{
			m_pScene->removeActor(*pActor);
		}
	}

	void PhysicsSystem::CreateCharacterCapsule(const CharacterColliderInfo& characterColliderInfo, float height, float radius, CharacterColliderComponent& characterColliderComp)
	{
		PxCapsuleControllerDesc controllerDesc = {};
		controllerDesc.radius			= radius;
		controllerDesc.height			= height;
		controllerDesc.climbingMode		= PxCapsuleClimbingMode::eEASY;

		FinalizeCharacterController(characterColliderInfo, controllerDesc, characterColliderComp);
	}

	void PhysicsSystem::CreateCharacterBox(const CharacterColliderInfo& characterColliderInfo, const glm::vec3& halfExtents, CharacterColliderComponent& characterColliderComp)
	{
		PxBoxControllerDesc controllerDesc = {};
		controllerDesc.halfHeight			= halfExtents.y;
		controllerDesc.halfSideExtent		= halfExtents.x;
		controllerDesc.halfForwardExtent	= halfExtents.z;

		FinalizeCharacterController(characterColliderInfo, controllerDesc, characterColliderComp);
	}

	glm::vec3 PhysicsSystem::GetCharacterTranslation(float32 dt, const glm::vec3& forward, const glm::vec3& right, const FPSControllerComponent& FPSComp)
	{
		// First calculate translation relative to the character's rotation (i.e. right, up, forward).
		// Then convert the translation be relative to the world axes.
		glm::vec3 translation = {
			float(InputActionSystem::IsActive("CAM_RIGHT")		- InputActionSystem::IsActive("CAM_LEFT")),		// X: Right
			0.0f,																								// Y: Up
			float(InputActionSystem::IsActive("CAM_FORWARD")	- InputActionSystem::IsActive("CAM_BACKWARD"))	// Z: Forward
		};

		if (glm::length2(translation) > glm::epsilon<float>())
		{
			const int isSprinting = InputActionSystem::IsActive("CAM_SPEED_MODIFIER");
			const float sprintFactor = std::max(1.0f, FPSComp.SprintSpeedFactor * isSprinting);
			translation = glm::normalize(translation) * FPSComp.SpeedFactor * sprintFactor * dt;
		}

		translation.y = -GRAVITATIONAL_ACCELERATION * dt;

		const glm::vec3 forwardHorizontal	= glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
		const glm::vec3 rightHorizontal		= glm::normalize(glm::vec3(right.x, 0.0f, right.z));

		return translation.x * rightHorizontal + translation.y * g_DefaultUp + translation.z * forwardHorizontal;
	}

	void PhysicsSystem::FinalizeCollisionComponent(const StaticCollisionInfo& collisionCreateInfo, PxShape* pShape, const PxQuat& additionalRotation)
	{
		// Add filtering data
		PxFilterData filterData;
		filterData.word0 = (PxU32)collisionCreateInfo.CollisionGroup;
		filterData.word1 = (PxU32)collisionCreateInfo.CollisionMask;
		pShape->setSimulationFilterData(filterData);
		pShape->setQueryFilterData(filterData);

		// Combine shape and transform to create a static body
		const glm::vec3& position = collisionCreateInfo.Position.Position;
		const glm::quat& rotation = collisionCreateInfo.Rotation.Quaternion;

		const PxVec3 positionPX = { position.x, position.y, position.z };
		const PxQuat rotationPX = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w) * additionalRotation;
		const PxTransform transformPX(positionPX, rotationPX);

		PxRigidStatic* pBody = m_pPhysics->createRigidStatic(transformPX);
		pBody->attachShape(*pShape);

		m_pScene->addActor(*pBody);

		/*	Decreases the ref count to 1, which will drop to 0 either when explicitly removed, or when the scene
			is released */
		pShape->release();

		StaticCollisionComponent collisionComponent = { pBody };
		ECSCore::GetInstance()->AddComponent<StaticCollisionComponent>(collisionCreateInfo.Entity, collisionComponent);
	}

	void PhysicsSystem::FinalizeCharacterController(const CharacterColliderInfo& characterColliderInfo, PxControllerDesc& controllerDesc, CharacterColliderComponent& characterColliderComp)
	{
		/*	For information about PhysX character controllers in general:
			https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/CharacterControllers.html */

		/*	Max height of obstacles that can be climbed. Note that capsules can automatically climb obstacles because
			of their round bottoms, so the total step height is taller than the specified one below.
			This can be turned off however. */
		constexpr const float stepOffset = 0.20f;

		const glm::vec3& position = characterColliderInfo.Position.Position;
		const glm::vec3 upDirection = g_DefaultUp * characterColliderInfo.Rotation.Quaternion;

		controllerDesc.material			= m_pMaterial;
		controllerDesc.position			= { position.x, position.y, position.z };
		controllerDesc.upDirection		= { upDirection.x, upDirection.y, upDirection.z };
		controllerDesc.stepOffset		= stepOffset;
		controllerDesc.nonWalkableMode	= PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;

		PxController* pController = m_pControllerManager->createController(controllerDesc);
		PxFilterData* pFilterData = DBG_NEW PxFilterData(
			(PxU32)characterColliderInfo.CollisionGroup,
			(PxU32)characterColliderInfo.CollisionMask,
			0u,
			0u
		);

		PxControllerFilters controllerFilters(pFilterData);
		characterColliderComp.pController	= pController;
		characterColliderComp.Filters		= controllerFilters;
	}
}
