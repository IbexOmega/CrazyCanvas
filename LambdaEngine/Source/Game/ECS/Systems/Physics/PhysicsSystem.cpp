#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Input/API/InputActionSystem.h"
#include "Physics/PhysX/FilterShader.h"
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
		// Register system
		{
			auto onStaticCollisionAdded = std::bind_front(&PhysicsSystem::OnStaticCollisionAdded, this);
			auto onStaticCollisionRemoval = std::bind_front(&PhysicsSystem::OnStaticCollisionRemoval, this);

			auto onDynamicCollisionAdded = std::bind_front(&PhysicsSystem::OnDynamicCollisionAdded, this);
			auto onDynamicCollisionRemoval = std::bind_front(&PhysicsSystem::OnDynamicCollisionRemoval, this);
			auto onCharacterCollisionRemoval = std::bind_front(&PhysicsSystem::OnCharacterColliderRemoval, this);

			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					.pSubscriber = &m_StaticCollisionEntities,
					.ComponentAccesses =
					{
						{NDA, StaticCollisionComponent::Type()}, {NDA, PositionComponent::Type()}, {NDA, RotationComponent::Type()}
					},
					.OnEntityAdded = onStaticCollisionAdded,
					.OnEntityRemoval = onStaticCollisionRemoval
				},
				{
					.pSubscriber = &m_DynamicCollisionEntities,
					.ComponentAccesses =
					{
						{R, DynamicCollisionComponent::Type()}, {RW, PositionComponent::Type()}, {RW, RotationComponent::Type()}, {RW, VelocityComponent::Type()}
					},
					.OnEntityAdded = onDynamicCollisionAdded,
					.OnEntityRemoval = onDynamicCollisionRemoval
				},
				{
					.pSubscriber = &m_CharacterCollisionEntities,
					.ComponentAccesses =
					{
						{RW, CharacterColliderComponent::Type()}
					},
					.OnEntityRemoval = onCharacterCollisionRemoval
				}
			};
			systemReg.Phase = 1;

			RegisterSystem(TYPE_NAME(PhysicsSystem), systemReg);

			SetComponentOwner<StaticCollisionComponent>({ .Destructor = &PhysicsSystem::StaticCollisionDestructor });
			SetComponentOwner<DynamicCollisionComponent>({ .Destructor = &PhysicsSystem::DynamicCollisionDestructor });
			SetComponentOwner<CharacterColliderComponent>({ .Destructor = &PhysicsSystem::CharacterColliderDestructor });
		}

		// PhysX setup
		m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_Allocator, m_ErrorCallback);
		if (!m_pFoundation)
		{
			LOG_ERROR("PhysX foundation creation failed");
			return false;
		}

	#ifdef LAMBDA_DEBUG
		if (EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_STREAM_PHYSX))
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

		const glm::vec3 gravity = GRAVITATIONAL_ACCELERATION * -g_DefaultUp;
		const PxVec3 gravityPX = { gravity.x, gravity.y, gravity.z };

		PxSceneDesc sceneDesc(m_pPhysics->getTolerancesScale());
		sceneDesc.flags						= PxSceneFlag::eENABLE_CCD;
		sceneDesc.gravity					= gravityPX;
		sceneDesc.cpuDispatcher				= m_pDispatcher;
		sceneDesc.filterShader				= FilterShader;
		sceneDesc.simulationEventCallback	= this;
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

		ECSCore* pECS = ECSCore::GetInstance();
		const ComponentArray<DynamicCollisionComponent>* pDynamicCollisionComponents = pECS->GetComponentArray<DynamicCollisionComponent>();
		ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_DynamicCollisionEntities)
		{
			const DynamicCollisionComponent& collisionComp = pDynamicCollisionComponents->GetConstData(entity);
			PxRigidDynamic* pActor = collisionComp.pActor;
			if (!pActor->isSleeping())
			{
				PositionComponent& positionComp = pPositionComponents->GetData(entity);
				RotationComponent& rotationComp = pRotationComponents->GetData(entity);
				VelocityComponent& velocityComp = pVelocityComponents->GetData(entity);

				const PxTransform transformPX = pActor->getGlobalPose();
				const PxVec3& positionPX = transformPX.p;
				positionComp.Position = { positionPX.x, positionPX.y, positionPX.z };

				const PxQuat& quatPX = transformPX.q;
				rotationComp.Quaternion = { quatPX.x, quatPX.y, quatPX.z, quatPX.w };

				const PxVec3 velocityPX = pActor->getLinearVelocity();
				velocityComp.Velocity = { velocityPX.x, velocityPX.y, velocityPX.z };
			}
		}
	}

	StaticCollisionComponent PhysicsSystem::CreateStaticActor(const CollisionCreateInfo& collisionInfo)
	{
		StaticCollisionComponent collisionComponent = FinalizeStaticCollisionActor(collisionInfo);

		for (const ShapeCreateInfo& shapeCreateInfo : collisionInfo.Shapes)
		{
			PxShape* pShape = CreateShape(
				shapeCreateInfo, 
				collisionInfo.Position.Position, 
				collisionInfo.Scale.Scale, 
				collisionInfo.Rotation.Quaternion);

			if (pShape != nullptr)
			{
				collisionComponent.pActor->attachShape(*pShape);

				// Decreases the ref count to 1, which will drop to 0 when the actor is deleted
				pShape->release();
			}
		}

		return collisionComponent;
	}

	DynamicCollisionComponent PhysicsSystem::CreateDynamicActor(const DynamicCollisionCreateInfo& collisionInfo)
	{
		DynamicCollisionComponent collisionComponent = FinalizeDynamicCollisionActor(collisionInfo);

		for (const ShapeCreateInfo& shapeCreateInfo : collisionInfo.Shapes)
		{
			PxShape* pShape = CreateShape(
				shapeCreateInfo, 
				collisionInfo.Position.Position, 
				collisionInfo.Scale.Scale, 
				collisionInfo.Rotation.Quaternion);

			if (pShape != nullptr)
			{
				collisionComponent.pActor->attachShape(*pShape);

				// Decreases the ref count to 1, which will drop to 0 when the actor is deleted
				pShape->release();
			}
		}

		return collisionComponent;
	}

	CharacterColliderComponent PhysicsSystem::CreateCharacterCapsule(
		const CharacterColliderCreateInfo& characterColliderInfo, 
		float32 height, 
		float32 radius)
	{
		PxCapsuleControllerDesc controllerDesc = {};
		controllerDesc.radius			= radius;
		controllerDesc.height			= height;
		controllerDesc.climbingMode		= PxCapsuleClimbingMode::eCONSTRAINED;

		return FinalizeCharacterController(characterColliderInfo, controllerDesc);
	}

	CharacterColliderComponent PhysicsSystem::CreateCharacterBox(
		const CharacterColliderCreateInfo& characterColliderInfo, 
		const glm::vec3& halfExtents)
	{
		PxBoxControllerDesc controllerDesc = {};
		controllerDesc.halfHeight			= halfExtents.y;
		controllerDesc.halfSideExtent		= halfExtents.x;
		controllerDesc.halfForwardExtent	= halfExtents.z;

		return FinalizeCharacterController(characterColliderInfo, controllerDesc);
	}

	float32 PhysicsSystem::CalculateSphereRadius(const Mesh* pMesh)
	{
		const TArray<Vertex>& vertices = pMesh->Vertices;
		float squareRadius = 0.0f;

		for (const Vertex& vertex : vertices)
		{
			squareRadius = std::max(squareRadius, glm::length2(vertex.Position));
		}

		return std::sqrtf(squareRadius);
	}

	void PhysicsSystem::CalculateCapsuleDimensions(Mesh* pMesh, float32& radius, float32& halfHeight)
	{
		/*	A PhysX capsule's height extends along the x-axis. To make the capsule stand upright,
			it is rotated around the z-axis. */
		const TArray<Vertex>& vertices = pMesh->Vertices;

		// The radius in the XZ plane (horizontal)
		float32 squareRadiusXZ = 0.0f;

		for (const Vertex& vertex : vertices)
		{
			const glm::vec3& position = vertex.Position;
			squareRadiusXZ = std::max(squareRadiusXZ, glm::length2(glm::vec3(position.x, 0.0f, position.z)));
			halfHeight = std::max(halfHeight, std::abs(position.y));
		}

		radius = std::sqrtf(squareRadiusXZ);
		halfHeight = halfHeight - radius;
	}

	void PhysicsSystem::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pPairs, PxU32 nbPairs)
	{
		for (PxU32 pairIdx = 0; pairIdx < nbPairs; pairIdx++)
		{
			const PxContactPair& contactPair = pPairs[pairIdx];

			TArray<PxContactPairPoint> contactPoints(contactPair.contactCount);
			if (contactPair.events & (PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_CONTACT_POINTS))
			{
				contactPair.extractContacts(contactPoints.GetData(), contactPair.contactCount);
				CollisionCallbacks({ pairHeader.actors[0], pairHeader.actors[1] }, { contactPair.shapes[0], contactPair.shapes[1] }, contactPoints);
			}
		}
	}

	void PhysicsSystem::onTrigger(PxTriggerPair* pTriggerPairs, PxU32 nbPairs)
	{
		for (PxU32 pairIdx = 0; pairIdx < nbPairs; pairIdx++)
		{
			const PxTriggerPair& triggerPair = pTriggerPairs[pairIdx];

			// Ignore pairs when shapes have been deleted
			if (triggerPair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
			{
				continue;
			}

			TriggerCallbacks({ triggerPair.triggerActor, triggerPair.otherActor }, { triggerPair .triggerShape, triggerPair.otherShape });
		}
	}

	PxShape* PhysicsSystem::CreateShape(
		const ShapeCreateInfo& shapeCreateInfo, 
		const glm::vec3& position, 
		const glm::vec3& scale, 
		const glm::quat& rotation) const
	{
		PxShape* pShape = nullptr;

		switch (shapeCreateInfo.GeometryType)
		{
			case EGeometryType::SPHERE:
			{
				const float32 maxScale = glm::compMax(scale);
				pShape = m_pPhysics->createShape(PxSphereGeometry(shapeCreateInfo.GeometryParams.Radius * maxScale), *m_pMaterial);
				break;
			}
			case EGeometryType::BOX:
			{
				const PxVec3 halfExtentsPX =
				{
					scale.x * shapeCreateInfo.GeometryParams.HalfExtents.x,
					scale.y * shapeCreateInfo.GeometryParams.HalfExtents.y,
					scale.z * shapeCreateInfo.GeometryParams.HalfExtents.z
				};
				pShape = m_pPhysics->createShape(PxBoxGeometry(halfExtentsPX), *m_pMaterial);
				break;
			}
			case EGeometryType::CAPSULE:
			{
				pShape = CreateCollisionCapsule(shapeCreateInfo.GeometryParams.Radius, shapeCreateInfo.GeometryParams.HalfHeight);

				// Rotate around Z-axis to get the capsule pointing upwards
				const glm::quat uprightRotation = glm::rotate(glm::identity<glm::quat>(), glm::half_pi<float32>() * g_DefaultForward);
				const PxTransform transformPX = CreatePxTransform(glm::vec3(0.0f), uprightRotation);
				pShape->setLocalPose(transformPX);
				break;
			}
			case EGeometryType::PLANE:
			{
				/*	PhysX treats the planes' x-axis as their normals, whilst the PhysicsSystem API says the Y-axis is
					the normal. Set a local pose for the shape to make the Y-axis the plane normal. */
				const glm::vec3 planeNormal = GetUp(rotation);
				const PxPlane plane({ position.x, position.y, position.z }, { planeNormal.x, planeNormal.y, planeNormal.z });
				const PxTransform transformPX = PxTransformFromPlaneEquation(plane);

				pShape = m_pPhysics->createShape(PxPlaneGeometry(), *m_pMaterial);
				pShape->setLocalPose(transformPX);
				break;
			}
			case EGeometryType::MESH:
			{
				pShape = CreateCollisionTriangleMesh(shapeCreateInfo.GeometryParams.pMesh, scale);
				break;
			}
		}

		if (pShape != nullptr)
		{
			// Set shape's filter data
			PxFilterData filterData;
			filterData.word0 = (PxU32)shapeCreateInfo.CollisionGroup;
			filterData.word1 = (PxU32)shapeCreateInfo.CollisionMask;
			filterData.word2 = (PxU32)shapeCreateInfo.EntityID;
			pShape->setSimulationFilterData(filterData);
			
			filterData.word2 = 0;
			pShape->setQueryFilterData(filterData);

			if (shapeCreateInfo.ShapeType == EShapeType::TRIGGER)
			{
				pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
				pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
			}
			else if (shapeCreateInfo.ShapeType == EShapeType::SIMULATION)
			{
				pShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
				pShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
			}

			// Set shape user data
			ShapeUserData* pShapeUserData = DBG_NEW ShapeUserData;
			pShapeUserData->CallbackFunction = shapeCreateInfo.CallbackFunction;

			if (shapeCreateInfo.pUserData != nullptr && shapeCreateInfo.UserDataSize > 0)
			{
				pShapeUserData->pUserData = DBG_NEW byte[shapeCreateInfo.UserDataSize];
				memcpy(pShapeUserData->pUserData, shapeCreateInfo.pUserData, shapeCreateInfo.UserDataSize);
			}

			pShape->userData = pShapeUserData;
		}

		return pShape;
	}

	PxShape* PhysicsSystem::CreateCollisionCapsule(float32 radius, float32 halfHeight) const
	{
		/*	A PhysX capsule's height extends along the x-axis. To make the capsule stand upright,
			it is rotated around the z-axis. */
		PxShape* pShape = nullptr;
		if (halfHeight > 0.0f)
		{
			pShape = m_pPhysics->createShape(PxCapsuleGeometry(radius, halfHeight), *m_pMaterial);
		}
		else
		{
			pShape = m_pPhysics->createShape(PxSphereGeometry(radius), *m_pMaterial);
		}

		return pShape;
	}

	PxShape* PhysicsSystem::CreateCollisionTriangleMesh(const Mesh* pMesh, const glm::vec3& scale) const
	{
		/* Perform mesh 'cooking'; generate an optimized collision mesh from triangle data */
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
		PxTriangleMeshGeometry triangleMeshGeometry(pTriangleMesh, PxMeshScale({ scale.x, scale.y, scale.z }));
		return m_pPhysics->createShape(triangleMeshGeometry, *m_pMaterial);
	}

	PxTransform PhysicsSystem::CreatePxTransform(const glm::vec3& position, const glm::quat& rotation) const
	{
		const PxVec3 positionPX = { position.x, position.y, position.z };
		const PxQuat rotationPX = PxQuat(rotation.x, rotation.y, rotation.z, rotation.w);
		return PxTransform(positionPX, rotationPX);
	}

	void PhysicsSystem::StaticCollisionDestructor(StaticCollisionComponent& collisionComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		ReleaseActor(collisionComponent.pActor);
		collisionComponent.pActor = nullptr;
	}

	void PhysicsSystem::DynamicCollisionDestructor(DynamicCollisionComponent& collisionComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		ReleaseActor(collisionComponent.pActor);
		collisionComponent.pActor = nullptr;
	}

	void PhysicsSystem::CharacterColliderDestructor(CharacterColliderComponent& characterColliderComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		PxRigidDynamic* pActor = characterColliderComponent.pController->getActor();
		
		TArray<PxShape*> pxShapes(pActor->getNbShapes());
		pActor->getShapes(pxShapes.GetData(), pxShapes.GetSize());

		for (PxShape* pShape : pxShapes)
		{
			if (pShape->userData != nullptr)
			{
				ShapeUserData* pShapeUserData = reinterpret_cast<ShapeUserData*>(pShape->userData);
				free(pShapeUserData->pUserData);
				delete pShapeUserData;

				pShape->userData = nullptr;
			}
		}

		delete reinterpret_cast<ActorUserData*>(pActor->userData);
		pActor->userData = nullptr;
		PX_RELEASE(characterColliderComponent.pController);
		SAFEDELETE(characterColliderComponent.Filters.mFilterData);
	}

	void PhysicsSystem::ReleaseActor(PxRigidActor* pActor)
	{
		if (pActor)
		{
			delete reinterpret_cast<ActorUserData*>(pActor->userData);
			pActor->userData = nullptr;

			TArray<PxShape*> pxShapes(pActor->getNbShapes());
			pActor->getShapes(pxShapes.GetData(), pxShapes.GetSize());

			for (PxShape* pShape : pxShapes)
			{
				if (pShape->userData != nullptr)
				{
					ShapeUserData* pShapeUserData = reinterpret_cast<ShapeUserData*>(pShape->userData);
					free(pShapeUserData->pUserData);
					delete pShapeUserData;

					pShape->userData = nullptr;
				}
			}

			pActor->release();
		}
	}

	void PhysicsSystem::OnStaticCollisionAdded(Entity entity)
	{
		StaticCollisionComponent& collisionComp = ECSCore::GetInstance()->GetComponent<StaticCollisionComponent>(entity);
		m_pScene->addActor(*collisionComp.pActor);
	}

	void PhysicsSystem::OnDynamicCollisionAdded(Entity entity)
	{
		DynamicCollisionComponent& collisionComp = ECSCore::GetInstance()->GetComponent<DynamicCollisionComponent>(entity);
		m_pScene->addActor(*collisionComp.pActor);
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

	StaticCollisionComponent PhysicsSystem::FinalizeStaticCollisionActor(
		const CollisionCreateInfo& collisionInfo, 
		const glm::quat& additionalRotation)
	{
		const glm::vec3& position = collisionInfo.Position.Position;
		const glm::quat rotation = collisionInfo.Rotation.Quaternion * additionalRotation;
		const PxTransform transformPX = CreatePxTransform(position, rotation);
		const PxPlane plane = PxPlaneEquationFromTransform(transformPX);

		PxRigidStatic* pActor = m_pPhysics->createRigidStatic(transformPX);
		FinalizeCollisionActor(collisionInfo, pActor);

		return { pActor };
	}

	DynamicCollisionComponent PhysicsSystem::FinalizeDynamicCollisionActor(
		const DynamicCollisionCreateInfo& collisionInfo, 
		const glm::quat& additionalRotation)
	{
		const glm::vec3& position = collisionInfo.Position.Position;
		const glm::quat rotation = collisionInfo.Rotation.Quaternion * additionalRotation;
		const PxTransform transformPX = CreatePxTransform(position, rotation);

		const glm::vec3& initialVelocity = collisionInfo.Velocity.Velocity;
		const PxVec3 initialVelocityPX = { initialVelocity.x, initialVelocity.y, initialVelocity.z };

		PxRigidDynamic* pActor = m_pPhysics->createRigidDynamic(transformPX);
		pActor->setLinearVelocity(initialVelocityPX);
		FinalizeCollisionActor(collisionInfo, pActor);

		return { pActor };
	}

	CharacterColliderComponent PhysicsSystem::FinalizeCharacterController(
		const CharacterColliderCreateInfo& characterColliderInfo, 
		PxControllerDesc& controllerDesc)
	{
		/*	For information about PhysX character controllers in general:
			https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/CharacterControllers.html */

		/*	Max height of obstacles that can be climbed. Note that capsules can automatically climb obstacles because
			of their round bottoms, so the total step height is taller than the specified one below.
			This can be turned off however. */
		constexpr const float STEP_OFFSET = 0.20f;

		const glm::vec3& position = characterColliderInfo.Position.Position;
		const glm::vec3 upDirection = g_DefaultUp * glm::quat(characterColliderInfo.Rotation.Quaternion.w, 0.0f, characterColliderInfo.Rotation.Quaternion.y, 0.0f);

		controllerDesc.material			= m_pMaterial;
		controllerDesc.position			= { position.x, position.y, position.z };
		controllerDesc.upDirection		= { upDirection.x, upDirection.y, upDirection.z };
		controllerDesc.stepOffset		= STEP_OFFSET;
		controllerDesc.nonWalkableMode	= PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;

		PxController* pController = m_pControllerManager->createController(controllerDesc);
		pController->setFootPosition(controllerDesc.position);

		// Set filter data to be used when calling controller::move()
		PxFilterData* pFilterData = DBG_NEW PxFilterData(
			(PxU32)characterColliderInfo.CollisionGroup,
			(PxU32)characterColliderInfo.CollisionMask,
			(PxU32)characterColliderInfo.EntityID,
			0u
		);

		PxControllerFilters controllerFilters(pFilterData);

		// Set filter data to be used when simulating the physics world
		PxRigidDynamic* pActor = pController->getActor();
		PxShape* pShape = nullptr;
		pActor->getShapes(&pShape, 1, 0);

		PxFilterData filterData;
		filterData.word0 = (PxU32)characterColliderInfo.CollisionGroup;
		filterData.word1 = (PxU32)characterColliderInfo.CollisionMask;
		filterData.word2 = (PxU32)characterColliderInfo.EntityID;
		pShape->setSimulationFilterData(filterData);

		pShape->userData = DBG_NEW ShapeUserData();

		// Set actor's user data
		ActorUserData* pActorUserData = DBG_NEW ActorUserData;
		pActorUserData->Entity = characterColliderInfo.Entity;
		pActor->userData = pActorUserData;

		return { pController, controllerFilters };
	}

	void PhysicsSystem::FinalizeCollisionActor(const CollisionCreateInfo& collisionInfo, PxRigidActor* pActor)
	{
		if (pActor->is<PxRigidBody>() && collisionInfo.DetectionMethod == ECollisionDetection::CONTINUOUS)
		{
			PxRigidBody* pBody = reinterpret_cast<PxRigidBody*>(pActor);
			pBody->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
		}

		// Set collision callback
		pActor->userData = DBG_NEW ActorUserData;
		ActorUserData* pUserData = reinterpret_cast<ActorUserData*>(pActor->userData);
		pUserData->Entity = collisionInfo.Entity;
	}

	void PhysicsSystem::TriggerCallbacks(const std::array<PxRigidActor*, 2>& actors, const std::array<PxShape*, 2>& shapes) const
	{
		ActorUserData* pActorUserDatas[2] =
		{
			reinterpret_cast<ActorUserData*>(actors[0]->userData),
			reinterpret_cast<ActorUserData*>(actors[1]->userData)
		};

		ShapeUserData* pShapeUserDatas[2] =
		{
			reinterpret_cast<ShapeUserData*>(shapes[0]->userData),
			reinterpret_cast<ShapeUserData*>(shapes[1]->userData)
		};

		const TriggerCallback* pTriggerCallback0 = std::get_if<TriggerCallback>(&pShapeUserDatas[0]->CallbackFunction);
		const TriggerCallback* pTriggerCallback1 = std::get_if<TriggerCallback>(&pShapeUserDatas[1]->CallbackFunction);

		if (pTriggerCallback0 && *pTriggerCallback0)
		{
			(*pTriggerCallback0)(pActorUserDatas[0]->Entity, pActorUserDatas[1]->Entity);
		}

		if (pTriggerCallback1 && *pTriggerCallback1)
		{
			(*pTriggerCallback1)(pActorUserDatas[1]->Entity, pActorUserDatas[0]->Entity);
		}
	}

	void PhysicsSystem::CollisionCallbacks(
		const std::array<PxRigidActor*, 2>& actors, 
		const std::array<PxShape*, 2>& shapes, 
		const TArray<PxContactPairPoint>& contactPoints) const
	{
		ActorUserData* pActorUserDatas[2] =
		{
			reinterpret_cast<ActorUserData*>(actors[0]->userData),
			reinterpret_cast<ActorUserData*>(actors[1]->userData)
		};

		ShapeUserData* pShapeUserDatas[2] =
		{
			reinterpret_cast<ShapeUserData*>(shapes[0]->userData),
			reinterpret_cast<ShapeUserData*>(shapes[1]->userData)
		};

		const CollisionCallback* pCollisionCallback0 = std::get_if<CollisionCallback>(&pShapeUserDatas[0]->CallbackFunction);
		const CollisionCallback* pCollisionCallback1 = std::get_if<CollisionCallback>(&pShapeUserDatas[1]->CallbackFunction);
		if (!pCollisionCallback0 && !*pCollisionCallback0 && !pCollisionCallback1 && !*pCollisionCallback1)
		{
			return;
		}

		// Take the first contact point. (We might want to change this to work for multiple contact points)
		const PxContactPairPoint& contactPoint = contactPoints[0];

		// At least one of the entities has a callback function. Create collision info for both entities.
		EntityCollisionInfo collisionInfos[2];
		for (uint32 actorIdx = 0; actorIdx < 2; actorIdx++)
		{
			const PxRigidActor* pActor = actors[actorIdx];

			/*	Get the direction of the actor. Default to the transform's rotation. If the actor is dynamic and has
				a non-zero velocity, use that instead. */
			const PxTransform transformPX = pActor->getGlobalPose();
			const glm::quat rotation = { transformPX.q.x, transformPX.q.y, transformPX.q.z, transformPX.q.w };
			glm::vec3 direction = GetForward(rotation);
			if (pActor->is<PxRigidDynamic>())
			{
				const PxRigidDynamic* pDynamicActor = reinterpret_cast<const PxRigidDynamic*>(pActor);
				const PxVec3 velocityPX = pDynamicActor->getLinearVelocity();
				if (!velocityPX.isZero())
				{
					direction = glm::normalize(glm::vec3(velocityPX.x, velocityPX.y, velocityPX.z));
				}
			}

			collisionInfos[actorIdx] =
			{
				.Entity = pActorUserDatas[actorIdx]->Entity,
				.Position = { contactPoint.position.x, contactPoint.position.y, contactPoint.position.z },
				.Direction = direction,
				.Normal = { contactPoint.normal.x, contactPoint.normal.y, contactPoint.normal.z }
			};
		}

		if (pCollisionCallback0 && *pCollisionCallback0)
		{
			(*pCollisionCallback0)(collisionInfos[0], collisionInfos[1]);
		}

		if (pCollisionCallback1 && *pCollisionCallback1)
		{
			(*pCollisionCallback1)(collisionInfos[1], collisionInfos[0]);
		}
	}
}
