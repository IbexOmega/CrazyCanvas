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
			auto onDynamicCollisionRemoval = std::bind(&PhysicsSystem::OnDynamicCollisionRemoval, this, std::placeholders::_1);
			auto onCharacterColliderRemoval = std::bind(&PhysicsSystem::OnCharacterColliderRemoval, this, std::placeholders::_1);

			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					.pSubscriber = &m_StaticCollisionEntities,
					.ComponentAccesses =
					{
						{NDA, StaticCollisionComponent::Type()}, {NDA, PositionComponent::Type()}, {NDA, RotationComponent::Type()}
					},
					.OnEntityRemoval = onStaticCollisionRemoval
				},
				{
					.pSubscriber = &m_DynamicCollisionEntities,
					.ComponentAccesses =
					{
						{NDA, DynamicCollisionComponent::Type()}, {RW, PositionComponent::Type()}, {RW, RotationComponent::Type()}, {RW, VelocityComponent::Type()}
					},
					.OnEntityRemoval = onDynamicCollisionRemoval
				},
				{
					.pSubscriber = &m_CharacterColliderEntities,
					.ComponentAccesses =
					{
						{RW, CharacterColliderComponent::Type()}, {RW, PositionComponent::Type()}, {RW, VelocityComponent::Type()}
					},
					.OnEntityRemoval = onCharacterColliderRemoval
				}
			};
			systemReg.Phase = 1;

			RegisterSystem(systemReg);
			SetComponentOwner<StaticCollisionComponent>({ std::bind(&PhysicsSystem::StaticCollisionDestructor, this, std::placeholders::_1) });
			SetComponentOwner<DynamicCollisionComponent>({ std::bind(&PhysicsSystem::DynamicCollisionDestructor, this, std::placeholders::_1) });
			SetComponentOwner<CharacterColliderComponent>({ std::bind(&PhysicsSystem::CharacterColliderDestructor, this, std::placeholders::_1) });
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
		if (dt > 0.5f)
		{
			return;
		}

		TickCharacterControllers(dt);

		m_pScene->simulate(dt);
		m_pScene->fetchResults(true);

		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<DynamicCollisionComponent>* pDynamicCollisionComponents = pECS->GetComponentArray<DynamicCollisionComponent>();
		ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_DynamicCollisionEntities)
		{
			const DynamicCollisionComponent& collisionComp = pDynamicCollisionComponents->GetData(entity);
			PxRigidDynamic* pActor = collisionComp.pActor;
			if (!pActor->isSleeping())
			{
				PositionComponent& positionComp = pPositionComponents->GetData(entity);
				RotationComponent& rotationComp = pRotationComponents->GetData(entity);
				VelocityComponent& velocityComp = pVelocityComponents->GetData(entity);

				const PxTransform transformPX = pActor->getGlobalPose();
				const PxVec3& positionPX = transformPX.p;
				positionComp.Position =
				{
					positionPX.x,
					positionPX.y,
					positionPX.z
				};

				const PxQuat& quatPX = transformPX.q;
				rotationComp.Quaternion =
				{
					quatPX.x,
					quatPX.y,
					quatPX.z,
					quatPX.w
				};

				const PxVec3 velocityPX = pActor->getLinearVelocity();
				velocityComp.Velocity =
				{
					velocityPX.x,
					velocityPX.y,
					velocityPX.z
				};
			}
		}
	}

	StaticCollisionComponent PhysicsSystem::CreateStaticCollisionSphere(const CollisionInfo& collisionInfo)
	{
		PxShape* pShape = CreateCollisionSphere(collisionInfo);
		return FinalizeStaticCollisionActor(collisionInfo, pShape);
	}

	StaticCollisionComponent PhysicsSystem::CreateStaticCollisionBox(const CollisionInfo& collisionInfo)
	{
		PxShape* pShape = CreateCollisionBox(collisionInfo);
		return FinalizeStaticCollisionActor(collisionInfo, pShape);
	}

	StaticCollisionComponent PhysicsSystem::CreateStaticCollisionCapsule(const CollisionInfo& collisionInfo)
	{
		PxShape* pShape = CreateCollisionCapsule(collisionInfo);

		// Rotate around Z-axis to get the capsule pointing upwards
		const glm::quat uprightRotation = glm::rotate(glm::identity<glm::quat>(), glm::half_pi<float32>() * g_DefaultForward);
		return FinalizeStaticCollisionActor(collisionInfo, pShape, uprightRotation);
	}

	StaticCollisionComponent PhysicsSystem::CreateStaticCollisionMesh(const CollisionInfo& collisionInfo)
	{
		PxShape* pShape = CreateCollisionTriangleMesh(collisionInfo);
		return FinalizeStaticCollisionActor(collisionInfo, pShape);
	}

	DynamicCollisionComponent PhysicsSystem::CreateDynamicCollisionSphere(const DynamicCollisionInfo& collisionInfo)
	{
		PxShape* pShape = CreateCollisionSphere(collisionInfo);
		return FinalizeDynamicCollisionActor(collisionInfo, pShape);
	}

	DynamicCollisionComponent PhysicsSystem::CreateDynamicCollisionBox(const DynamicCollisionInfo& collisionInfo)
	{
		PxShape* pShape = CreateCollisionBox(collisionInfo);
		return FinalizeDynamicCollisionActor(collisionInfo, pShape);
	}

	DynamicCollisionComponent PhysicsSystem::CreateDynamicCollisionCapsule(const DynamicCollisionInfo& collisionInfo)
	{
		PxShape* pShape = CreateCollisionCapsule(collisionInfo);

		// Rotate around Z-axis to get the capsule pointing upwards
		const glm::quat uprightRotation = glm::rotate(glm::identity<glm::quat>(), glm::half_pi<float32>() * g_DefaultForward);
		return FinalizeDynamicCollisionActor(collisionInfo, pShape, uprightRotation);
	}

	DynamicCollisionComponent PhysicsSystem::CreateDynamicCollisionMesh(const DynamicCollisionInfo& collisionInfo)
	{
		PxShape* pShape = CreateCollisionTriangleMesh(collisionInfo);
		return FinalizeDynamicCollisionActor(collisionInfo, pShape);
	}

	CharacterColliderComponent PhysicsSystem::CreateCharacterCapsule(const CharacterColliderInfo& characterColliderInfo, float height, float radius)
	{
		PxCapsuleControllerDesc controllerDesc = {};
		controllerDesc.radius			= radius;
		controllerDesc.height			= height;
		controllerDesc.climbingMode		= PxCapsuleClimbingMode::eCONSTRAINED;

		return FinalizeCharacterController(characterColliderInfo, controllerDesc);
	}

	CharacterColliderComponent PhysicsSystem::CreateCharacterBox(const CharacterColliderInfo& characterColliderInfo, const glm::vec3& halfExtents)
	{
		PxBoxControllerDesc controllerDesc = {};
		controllerDesc.halfHeight			= halfExtents.y;
		controllerDesc.halfSideExtent		= halfExtents.x;
		controllerDesc.halfForwardExtent	= halfExtents.z;

		return FinalizeCharacterController(characterColliderInfo, controllerDesc);
	}

	PxShape* PhysicsSystem::CreateCollisionSphere(const CollisionInfo& staticCollisionInfo) const
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
		return pSphereShape;
	}

	PxShape* PhysicsSystem::CreateCollisionBox(const CollisionInfo& staticCollisionInfo) const
	{
		const Mesh* pMesh = ResourceManager::GetMesh(staticCollisionInfo.Mesh.MeshGUID);
		const glm::vec3& halfExtent = pMesh->BoundingBox.HalfExtent * staticCollisionInfo.Scale.Scale;
		const PxVec3 halfExtentPX(halfExtent.x, halfExtent.y, halfExtent.z);

		PxShape* pBoxShape = m_pPhysics->createShape(PxBoxGeometry(halfExtentPX), *m_pMaterial);
		return pBoxShape;
	}

	PxShape* PhysicsSystem::CreateCollisionCapsule(const CollisionInfo& staticCollisionInfo) const
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
		halfHeight = halfHeight - capsuleRadius;

		PxShape* pShape = nullptr;
		if (halfHeight > 0.0f)
		{
			pShape = m_pPhysics->createShape(PxCapsuleGeometry(capsuleRadius, halfHeight), *m_pMaterial);
		}
		else
		{
			pShape = m_pPhysics->createShape(PxSphereGeometry(capsuleRadius), *m_pMaterial);
		}

		return pShape;
	}

	PxShape* PhysicsSystem::CreateCollisionTriangleMesh(const CollisionInfo& staticCollisionInfo) const
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
			return nullptr;
		}

		PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		PxTriangleMesh* pTriangleMesh = m_pPhysics->createTriangleMesh(readBuffer);

		// Create a geometry instance of the mesh and scale it
		const glm::vec3 scale = staticCollisionInfo.Scale.Scale;

		PxTriangleMeshGeometry triangleMeshGeometry(pTriangleMesh, PxMeshScale({ scale.x, scale.y, scale.z }));
		return m_pPhysics->createShape(triangleMeshGeometry, *m_pMaterial);
	}

	void PhysicsSystem::SetFilterData(PxShape* pShape, const CollisionInfo& collisionInfo) const
	{
		PxFilterData filterData;
		filterData.word0 = (PxU32)collisionInfo.CollisionGroup;
		filterData.word1 = (PxU32)collisionInfo.CollisionMask;
		pShape->setSimulationFilterData(filterData);
		pShape->setQueryFilterData(filterData);
	}

	PxTransform PhysicsSystem::CreatePxTransform(const glm::vec3& position, const glm::quat& rotation) const
	{
		const PxVec3 positionPX = { position.x, position.y, position.z };
		const PxQuat rotationPX = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);
		return PxTransform(positionPX, rotationPX);
	}

	void PhysicsSystem::StaticCollisionDestructor(StaticCollisionComponent& collisionComponent)
	{
		PX_RELEASE(collisionComponent.pActor);
	}

	void PhysicsSystem::DynamicCollisionDestructor(DynamicCollisionComponent& collisionComponent)
	{
		PX_RELEASE(collisionComponent.pActor);
	}

	void PhysicsSystem::CharacterColliderDestructor(CharacterColliderComponent& characterColliderComponent)
	{
		PX_RELEASE(characterColliderComponent.pController);
		SAFEDELETE(characterColliderComponent.Filters.mFilterData);
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

	void PhysicsSystem::OnDynamicCollisionRemoval(Entity entity)
	{
		// Remove the actor from the scene
		DynamicCollisionComponent& collisionComponent = ECSCore::GetInstance()->GetComponent<DynamicCollisionComponent>(entity);
		PxRigidDynamic* pActor = collisionComponent.pActor;
		if (pActor)
		{
			m_pScene->removeActor(*pActor);
		}
	}

	void PhysicsSystem::OnCharacterColliderRemoval(Entity entity)
	{
		CharacterColliderComponent& characterCollider = ECSCore::GetInstance()->GetComponent<CharacterColliderComponent>(entity);
		PxActor* pActor = characterCollider.pController->getActor();
		if (pActor)
		{
			m_pScene->removeActor(*pActor);
		}
	}

	void PhysicsSystem::TickCharacterControllers(float32 dt)
	{
		// TODO: Temporary solution until there's a separate camera entity with an offset
		constexpr const float characterHeight = 1.8f;

		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<CharacterColliderComponent>* pCharacterColliders = pECS->GetComponentArray<CharacterColliderComponent>();
		ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_CharacterColliderEntities)
		{
			const PositionComponent& positionComp = pPositionComponents->GetData(entity);
			const glm::vec3& position = positionComp.Position;

			VelocityComponent& velocityComp = pVelocityComponents->GetData(entity);
			glm::vec3& velocity = velocityComp.Velocity;

			PxVec3 translationPX = { velocity.x, velocity.y, velocity.z };
			translationPX *= dt;

			CharacterColliderComponent& characterCollider = pCharacterColliders->GetData(entity);
			PxController* pController = characterCollider.pController;

			const PxExtendedVec3 oldPositionPX = pController->getPosition();

			if (positionComp.Dirty)
			{
				// Distance between the capsule's feet to its center position. Includes contact offset.
				const float32 capsuleHalfHeight = float32(oldPositionPX.y - pController->getFootPosition().y);
				pController->setPosition({ position.x, position.y - characterHeight + capsuleHalfHeight, position.z });
			}

			pController->move(translationPX, 0.0f, dt, characterCollider.Filters);

			const PxExtendedVec3& newPositionPX = pController->getPosition();
			const float32 oldVerticalSpeed = velocity.y;
			velocity = {
				newPositionPX.x - oldPositionPX.x,
				newPositionPX.y - oldPositionPX.y,
				newPositionPX.z - oldPositionPX.z
			};

			velocity /= dt;

			if (glm::length2(velocity) > glm::epsilon<float>())
			{
				// Prevent pController->move(...) from causing vertical acceleration because of stepping
				if (std::fabs(oldVerticalSpeed) < std::fabs(velocity.y))
				{
					velocity.y = oldVerticalSpeed;
				}

				// Update entity's position
				PositionComponent& positionCompMutable = const_cast<PositionComponent&>(positionComp);
				positionCompMutable.Dirty = true;
				glm::vec3& positionMutable = const_cast<glm::vec3&>(position);

				positionMutable = {
					newPositionPX.x,
					pController->getFootPosition().y + characterHeight,
					newPositionPX.z
				};
			}
		}
	}

	StaticCollisionComponent PhysicsSystem::FinalizeStaticCollisionActor(const CollisionInfo& collisionInfo, PxShape* pShape, const glm::quat& additionalRotation)
	{
		SetFilterData(pShape, collisionInfo);

		const glm::vec3& position = collisionInfo.Position.Position;
		const glm::quat rotation = collisionInfo.Rotation.Quaternion * additionalRotation;
		const PxTransform transformPX = CreatePxTransform(position, rotation);

		PxRigidStatic* pBody = m_pPhysics->createRigidStatic(transformPX);
		pBody->attachShape(*pShape);

		m_pScene->addActor(*pBody);

		/*	Decreases the ref count to 1, which will drop to 0 either when explicitly removed, or when the scene
			is released */
		pShape->release();

		return { pBody };
	}

	DynamicCollisionComponent PhysicsSystem::FinalizeDynamicCollisionActor(const DynamicCollisionInfo& collisionInfo, PxShape* pShape, const glm::quat& additionalRotation)
	{
		SetFilterData(pShape, collisionInfo);

		const glm::vec3& position = collisionInfo.Position.Position;
		const glm::quat rotation = collisionInfo.Rotation.Quaternion * additionalRotation;
		const PxTransform transformPX = CreatePxTransform(position, rotation);

		const glm::vec3& initialVelocity = collisionInfo.Velocity.Velocity;
		const PxVec3 initialVelocityPX =
		{
			initialVelocity.x,
			initialVelocity.y,
			initialVelocity.z
		};

		PxRigidDynamic* pBody = m_pPhysics->createRigidDynamic(transformPX);
		pBody->setLinearVelocity(initialVelocityPX);
		pBody->attachShape(*pShape);

		m_pScene->addActor(*pBody);

		/*	Decreases the ref count to 1, which will drop to 0 either when explicitly removed, or when the scene
			is released */
		pShape->release();

		return { pBody };
	}

	CharacterColliderComponent PhysicsSystem::FinalizeCharacterController(const CharacterColliderInfo& characterColliderInfo, PxControllerDesc& controllerDesc)
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
		return {
			.pController	= pController,
			.Filters		= controllerFilters
		};
	}
}
