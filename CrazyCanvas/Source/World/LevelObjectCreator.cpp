#include "World/LevelObjectCreator.h"
#include "World/KillPlane.h"
#include "World/Level.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "Game/ECS/Components/Audio/ListenerComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "ECS/ECSCore.h"
#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Teams/TeamHelper.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryEncoder.h"

#include "Math/Math.h"
#include "Math/Random.h"

#include "Resources/ResourceManager.h"

#include "Rendering/EntityMaskManager.h"

#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/Packet/PacketPlayerAction.h"
#include "Multiplayer/Packet/PacketPlayerActionResponse.h"
#include "Multiplayer/Packet/PacketFlagEdited.h"
#include "Multiplayer/Packet/PacketHealthChanged.h"
#include "Multiplayer/Packet/PacketWeaponFired.h"

#include "Physics/CollisionGroups.h"

bool LevelObjectCreator::Init()
{
	using namespace LambdaEngine;

	//Register Create Level Object by Prefix Functions
	{
		//Spawnpoint
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_PLAYER_SPAWN_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreatePlayerSpawn;
		}

		//Flag Spawn
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_FLAG_SPAWN_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreateFlagSpawn;
		}

		//Flag Delivery Point
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_FLAG_DELIVERY_POINT_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreateFlagDeliveryPoint;
		}

		//Kill plane
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_KILL_PLANE_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreateKillPlane;
		}
	}

	//Register Create Special Object by Type Functions
	{
		s_LevelObjectByTypeCreateFunctions[ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG]		= &LevelObjectCreator::CreateFlag;
		s_LevelObjectByTypeCreateFunctions[ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER]		= &LevelObjectCreator::CreatePlayer;
		s_LevelObjectByTypeCreateFunctions[ELevelObjectType::LEVEL_OBJECT_TYPE_PROJECTILE]	= &LevelObjectCreator::CreateProjectile;
	}

	//Load Object Meshes & Materials
	{
		//Flag
		{
			s_FlagMeshGUID		= ResourceManager::LoadMeshFromFile("Roller.obj");

			MaterialProperties materialProperties = {};
			materialProperties.Albedo = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

			s_FlagMaterialGUID = ResourceManager::LoadMaterialFromMemory(
				"Flag Material",
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_NORMAL_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				materialProperties);
		}

		//Player
		{
			s_PlayerMeshGUID					= ResourceManager::LoadMeshFromFile("Player/Idle.fbx", s_PlayerIdleGUIDs);

#ifndef LAMBDA_DEBUG
			s_PlayerRunGUIDs					= ResourceManager::LoadAnimationsFromFile("Player/Run.fbx");
			s_PlayerRunMirroredGUIDs			= ResourceManager::LoadAnimationsFromFile("Player/RunMirrored.fbx");
			s_PlayerRunBackwardGUIDs			= ResourceManager::LoadAnimationsFromFile("Player/RunBackward.fbx");
			s_PlayerRunBackwardMirroredGUIDs	= ResourceManager::LoadAnimationsFromFile("Player/RunBackwardMirrored.fbx");
			s_PlayerStrafeLeftGUIDs				= ResourceManager::LoadAnimationsFromFile("Player/StrafeLeft.fbx");
			s_PlayerStrafeRightGUIDs			= ResourceManager::LoadAnimationsFromFile("Player/StrafeRight.fbx");
#endif
		}
	}

	return true;
}

LambdaEngine::Entity LevelObjectCreator::CreateDirectionalLight(
	const LambdaEngine::LoadedDirectionalLight& directionalLight, 
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	Entity entity = UINT32_MAX;

	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();

		DirectionalLightComponent directionalLightComponent =
		{
			.ColorIntensity = directionalLight.ColorIntensity,
		};

		LOG_ERROR("LightDIRECTION: (%f, %f, %f)", directionalLight.Direction.x, directionalLight.Direction.y, directionalLight.Direction.z);

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, (translation) });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::quatLookAt({directionalLight.Direction}, g_DefaultUp) });
		pECS->AddComponent<DirectionalLightComponent>(entity, directionalLightComponent);

		D_LOG_INFO("[LevelObjectCreator]: Created Directional Light");
	}

	return entity;
}

LambdaEngine::Entity LevelObjectCreator::CreatePointLight(const LambdaEngine::LoadedPointLight& pointLight, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	Entity entity = UINT32_MAX;

	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();

		PointLightComponent pointLightComponent =
		{
			.ColorIntensity = pointLight.ColorIntensity
		};

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, (pointLight.Position + translation) });
		pECS->AddComponent<PointLightComponent>(entity, pointLightComponent);

		D_LOG_INFO("[LevelObjectCreator]: Created Point Light");
	}

	return entity;
}

LambdaEngine::Entity LevelObjectCreator::CreateStaticGeometry(const LambdaEngine::MeshComponent& meshComponent, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	Mesh* pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID);

	float32 maxDim = glm::max<float32>(
		pMesh->DefaultScale.x * pMesh->BoundingBox.Dimensions.x,
		glm::max<float32>(
			pMesh->DefaultScale.y * pMesh->BoundingBox.Dimensions.y,
			pMesh->DefaultScale.z * pMesh->BoundingBox.Dimensions.z));

	uint32 meshPaintSize = glm::max<uint32>(1, uint32(maxDim * 384.0f));

	ECSCore* pECS					= ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem	= PhysicsSystem::GetInstance();

	Entity entity = pECS->CreateEntity();
	pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "GeometryUnwrappedTexture", meshPaintSize, meshPaintSize));
	pECS->AddComponent<MeshComponent>(entity, meshComponent);
	const CollisionCreateInfo collisionCreateInfo =
	{
		.Entity			= entity,
		.Position		= pECS->AddComponent<PositionComponent>(entity, { true, pMesh->DefaultPosition + translation }),
		.Scale			= pECS->AddComponent<ScaleComponent>(entity,	{ true, pMesh->DefaultScale }),
		.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, pMesh->DefaultRotation }),
		.Shapes =
		{
			{
				/* ShapeType */			EShapeType::SIMULATION,
				/* GeometryType */		EGeometryType::MESH,
				/* Geometry */			{ .pMesh = pMesh },
				/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_STATIC,
				/* CollisionMask */		~FCollisionGroup::COLLISION_GROUP_STATIC, // Collide with any non-static object
				/* EntityID*/			entity
			},
		},
	};

	StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
	pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	return entity;
}

ELevelObjectType LevelObjectCreator::CreateLevelObjectFromPrefix(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	auto createFuncIt = s_LevelObjectByPrefixCreateFunctions.find(levelObject.Prefix);

	if (createFuncIt != s_LevelObjectByPrefixCreateFunctions.end())
	{
		return createFuncIt->second(levelObject, createdEntities, translation);
	}
	else
	{
		LOG_ERROR("[LevelObjectCreator]: Failed to create special object %s with prefix %s, no create function could be found", levelObject.Name.c_str(), levelObject.Prefix.c_str());
		return ELevelObjectType::LEVEL_OBJECT_TYPE_NONE;
	}
}

bool LevelObjectCreator::CreateLevelObjectOfType(
	ELevelObjectType levelObjectType,
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities)
{
	auto createFuncIt = s_LevelObjectByTypeCreateFunctions.find(levelObjectType);

	if (createFuncIt != s_LevelObjectByTypeCreateFunctions.end())
	{
		return createFuncIt->second(pData, createdEntities, createdChildEntities);
	}
	else
	{
		LOG_ERROR("[LevelObjectCreator]: Failed to create special object, no create function could be found");
		return false;
	}
}

ELevelObjectType LevelObjectCreator::CreatePlayerSpawn(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	if (levelObject.BoundingBoxes.GetSize() > 1 )
	{
		LOG_WARNING("[LevelObjectCreator]: Player Spawn can currently not be created with more than one mesh, using the first mesh...");
	}

	ECSCore* pECS = ECSCore::GetInstance();
	Entity entity = pECS->CreateEntity();

	PositionComponent& positionComponent = pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });
	ScaleComponent& scaleComponent = pECS->AddComponent<ScaleComponent>(entity, { true, levelObject.DefaultScale });
	RotationComponent& rotationComponent = pECS->AddComponent<RotationComponent>(entity, { true, levelObject.DefaultRotation });

	uint8 teamIndex = 0;
	size_t teamIndexPos = levelObject.Name.find("TEAM");
	if (teamIndexPos != String::npos) teamIndex = (uint8)std::stoi(levelObject.Name.substr(teamIndexPos + 4));

	pECS->AddComponent<TeamComponent>(entity, { .TeamIndex = teamIndex });

	if (!levelObject.MeshComponents.IsEmpty())
	{
		const MeshComponent& meshComponent = levelObject.MeshComponents[0];

		pECS->AddComponent<MeshComponent>(entity, meshComponent);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "GeometryUnwrappedTexture", 512, 512));

		PhysicsSystem* pPhysicsSystem	= PhysicsSystem::GetInstance();

		const CollisionCreateInfo collisionCreateInfo =
		{
			.Entity			= entity,
			.Position		= positionComponent,
			.Scale			= scaleComponent,
			.Rotation		= rotationComponent,
			.Shapes =
			{
				{
					/* ShapeType */			EShapeType::SIMULATION,
					/* GeometryType */		EGeometryType::MESH,
					/* Geometry */			{ .pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID) },
					/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_STATIC,
					/* CollisionMask */		~FCollisionGroup::COLLISION_GROUP_STATIC, // Collide with any non-static object
					/* EntityID*/			entity
				},
			},
		};

		StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
		pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	}

	createdEntities.PushBack(entity);

	D_LOG_INFO("Created Player Spawn with EntityID %u and Team Index %u", entity, teamIndex);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN;
}

ELevelObjectType LevelObjectCreator::CreateFlagSpawn(
	const LambdaEngine::LevelObjectOnLoad& levelObject,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Entity entity = pECS->CreateEntity();

	pECS->AddComponent<FlagSpawnComponent>(entity, { 1.0f });
	pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });

	createdEntities.PushBack(entity);

	D_LOG_INFO("Created Flag Spawn with EntityID %u", entity);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN;
}

ELevelObjectType LevelObjectCreator::CreateFlagDeliveryPoint(
	const LambdaEngine::LevelObjectOnLoad& levelObject, 
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, 
	const glm::vec3& translation)
{
	using namespace LambdaEngine;
	//Only the server is allowed to create a Base
	if (!MultiplayerUtils::IsServer())
		return ELevelObjectType::LEVEL_OBJECT_TYPE_NONE;

	if (levelObject.BoundingBoxes.GetSize() > 1)
	{
		LOG_WARNING("[LevelObjectCreator]: Bases can currently not be created with more than one Bounding Box, using the first Bounding Box...");
	}

	const BoundingBox& boundingBox = levelObject.BoundingBoxes[0];

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	Entity entity = pECS->CreateEntity();

	pECS->AddComponent<FlagDeliveryPointComponent>(entity, {});

	uint8 teamIndex = 0;
	size_t teamIndexPos = levelObject.Name.find("TEAM");
	if (teamIndexPos != String::npos) teamIndex = (uint8)std::stoi(levelObject.Name.substr(teamIndexPos + 4));

	pECS->AddComponent<TeamComponent>(entity, { .TeamIndex = teamIndex });
	const PositionComponent& positionComponent = pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });
	const ScaleComponent& scaleComponent = pECS->AddComponent<ScaleComponent>(entity, { true, levelObject.DefaultScale });
	const RotationComponent& rotationComponent = pECS->AddComponent<RotationComponent>(entity, { true, levelObject.DefaultRotation });

	//Only the server checks collision with the flag
	const CollisionCreateInfo collisionCreateInfo =
	{
		.Entity		= entity,
		.Position	= positionComponent,
		.Scale		= scaleComponent,
		.Rotation	= rotationComponent,
		.Shapes =
		{
			{
				/* Shape Type */		EShapeType::TRIGGER,
				/* GeometryType */		EGeometryType::BOX,
				/* Geometry */			{ .HalfExtents = boundingBox.Dimensions },
				/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG_DELIVERY_POINT,
				/* CollisionMask */		FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
				/* EntityID*/			entity
			},
		},
	};

	StaticCollisionComponent collisionComponent = pPhysicsSystem->CreateStaticActor(collisionCreateInfo);
	pECS->AddComponent<StaticCollisionComponent>(entity, collisionComponent);

	createdEntities.PushBack(entity);

	D_LOG_INFO("Created Base with EntityID %u and Team Index %u", entity, teamIndex);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_DELIVERY_POINT;
}

ELevelObjectType LevelObjectCreator::CreateKillPlane(
	const LambdaEngine::LevelObjectOnLoad& levelObject, 
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, 
	const glm::vec3& translation)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();
	const Entity entity = pECS->CreateEntity();
	const glm::vec3 boxHalfExtents = levelObject.BoundingBoxes.GetBack().Dimensions * levelObject.DefaultScale * 0.5f;

	const CollisionCreateInfo colliderInfo =
	{
		.Entity		= entity,
		.Position	= pECS->AddComponent<PositionComponent>(entity,	{ true, levelObject.DefaultPosition + translation }),
		.Scale		= pECS->AddComponent<ScaleComponent>(entity,	{ true, levelObject.DefaultScale }),
		.Rotation	= pECS->AddComponent<RotationComponent>(entity,	{ true, glm::identity<glm::quat>() }),
		.Shapes	=
		{
			{
				.ShapeType			= EShapeType::TRIGGER,
				.GeometryType		= EGeometryType::PLANE,
				.CollisionGroup		= FCollisionGroup::COLLISION_GROUP_STATIC,
				.CollisionMask		= ~FCollisionGroup::COLLISION_GROUP_STATIC,
				.EntityID			= entity,
				.CallbackFunction	= &KillPlaneCallback
			}
		}
	};

	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	const StaticCollisionComponent staticCollider = pPhysicsSystem->CreateStaticActor(colliderInfo);
	pECS->AddComponent<StaticCollisionComponent>(entity, staticCollider);
	createdEntities.PushBack(entity);

	return ELevelObjectType::LEVEL_OBJECT_TYPE_KILL_PLANE;
}

bool LevelObjectCreator::CreateFlag(
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities)
{
	UNREFERENCED_VARIABLE(createdChildEntities);

	if (pData == nullptr) return false;

	using namespace LambdaEngine;

	const CreateFlagDesc* pFlagDesc = reinterpret_cast<const CreateFlagDesc*>(pData);

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	Entity flagEntity = pECS->CreateEntity();

	const Timestamp			pickupCooldown = Timestamp::Seconds(1.0f);
	const FlagComponent		flagComponent{ EngineLoop::GetTimeSinceStart() + pickupCooldown, pickupCooldown };
	const PositionComponent	positionComponent{ true, pFlagDesc->Position };
	const ScaleComponent	scaleComponent{ true, pFlagDesc->Scale };
	const RotationComponent	rotationComponent{ true, pFlagDesc->Rotation };
	const MeshComponent		meshComponent{ s_FlagMeshGUID, s_FlagMaterialGUID };

	pECS->AddComponent<FlagComponent>(flagEntity,		flagComponent);
	pECS->AddComponent<PositionComponent>(flagEntity,	positionComponent);
	pECS->AddComponent<ScaleComponent>(flagEntity,		scaleComponent);
	pECS->AddComponent<RotationComponent>(flagEntity,	rotationComponent);
	pECS->AddComponent<MeshComponent>(flagEntity,		meshComponent);

	bool attachedToParent = pFlagDesc->ParentEntity != UINT32_MAX;

	ParentComponent parentComponent =
	{
		.Parent		= pFlagDesc->ParentEntity,
		.Attached	= attachedToParent,
	};

	OffsetComponent offsetComponent =
	{
		.Offset		= glm::vec3(0.0f)
	};

	if (attachedToParent)
	{
		const CharacterColliderComponent& parentCollisionComponent = pECS->GetConstComponent<CharacterColliderComponent>(pFlagDesc->ParentEntity);

		//Set Flag Offset
		const physx::PxBounds3& parentBoundingBox = parentCollisionComponent.pController->getActor()->getWorldBounds();
		offsetComponent.Offset = glm::vec3(0.0f, parentBoundingBox.getDimensions().y / 2.0f, 0.0f);
	}

	pECS->AddComponent<ParentComponent>(flagEntity,	parentComponent);
	pECS->AddComponent<OffsetComponent>(flagEntity,	offsetComponent);

	//Network Stuff
	{
		pECS->AddComponent<PacketComponent<PacketFlagEdited>>(flagEntity, {});
	}

	int32 networkUID;
	if (!MultiplayerUtils::IsServer())
	{
		networkUID = pFlagDesc->NetworkUID;
	}
	else
	{
		EFlagColliderType flagPlayerColliderType = EFlagColliderType::FLAG_COLLIDER_TYPE_PLAYER;
		EFlagColliderType flagDeliveryPointColliderType = EFlagColliderType::FLAG_COLLIDER_TYPE_DELIVERY_POINT;

		//Only the server checks collision with the flag
		const Mesh* pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID);
		const DynamicCollisionCreateInfo collisionCreateInfo =
		{
			/* Entity */	 		flagEntity,
            /* Detection Method */	ECollisionDetection::DISCRETE,
			/* Position */	 		positionComponent,
			/* Scale */				scaleComponent,
			/* Rotation */			rotationComponent,
			{
				{
					/* Shape Type */		EShapeType::TRIGGER,
					/* GeometryType */		EGeometryType::BOX,
					/* Geometry */			{ .HalfExtents = pMesh->BoundingBox.Dimensions },
					/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
					/* CollisionMask */		FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
					/* EntityID*/			flagEntity,
					/* CallbackFunction */	std::bind_front(&FlagSystemBase::OnPlayerFlagCollision, FlagSystemBase::GetInstance()),
					/* UserData */			&flagPlayerColliderType,
					/* UserDataSize */		sizeof(EFlagColliderType)
				},
				{
					/* Shape Type */		EShapeType::SIMULATION,
					/* GeometryType */		EGeometryType::BOX,
					/* Geometry */			{ .HalfExtents = pMesh->BoundingBox.Dimensions },
					/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
					/* CollisionMask */		FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG_DELIVERY_POINT,
					/* EntityID*/			flagEntity,
					/* CallbackFunction */	std::bind_front(&FlagSystemBase::OnDeliveryPointFlagCollision, FlagSystemBase::GetInstance()),
					/* UserData */			&flagDeliveryPointColliderType,
					/* UserDataSize */		sizeof(EFlagColliderType)
				},
			},
			/* Velocity */			pECS->AddComponent<VelocityComponent>(flagEntity, { glm::vec3(0.0f) })
		};

		DynamicCollisionComponent collisionComponent = pPhysicsSystem->CreateDynamicActor(collisionCreateInfo);
		collisionComponent.pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		pECS->AddComponent<DynamicCollisionComponent>(flagEntity, collisionComponent);

		networkUID = (int32)flagEntity;
	}

	pECS->AddComponent<NetworkComponent>(flagEntity, { networkUID });

	createdEntities.PushBack(flagEntity);

	D_LOG_INFO("Created Flag with EntityID %u and NetworkID %u", flagEntity, networkUID);
	return true;
}

bool LevelObjectCreator::CreatePlayer(
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities)
{
	if (pData == nullptr)
		return false;

	using namespace LambdaEngine;

	const CreatePlayerDesc* pPlayerDesc = reinterpret_cast<const CreatePlayerDesc*>(pData);

	ECSCore* pECS = ECSCore::GetInstance();
	const Entity playerEntity = pECS->CreateEntity();
	createdEntities.PushBack(playerEntity);
	TArray<Entity>& childEntities = createdChildEntities.PushBack({});

	const glm::quat lookDirQuat = glm::quatLookAt(pPlayerDesc->Forward, g_DefaultUp);

	pECS->AddComponent<PlayerBaseComponent>(playerEntity,		PlayerBaseComponent());
	EntityMaskManager::AddExtensionToEntity(playerEntity,		PlayerBaseComponent::Type(), nullptr);

	pECS->AddComponent<PositionComponent>(playerEntity,			PositionComponent{ .Position = pPlayerDesc->Position });
	pECS->AddComponent<NetworkPositionComponent>(playerEntity,
		NetworkPositionComponent
		{
			.Position		= pPlayerDesc->Position,
			.PositionLast	= pPlayerDesc->Position,
			.TimestampStart = EngineLoop::GetTimeSinceStart(),
			.Duration		= EngineLoop::GetFixedTimestep()
		});

	pECS->AddComponent<RotationComponent>(playerEntity,			RotationComponent{ .Quaternion = lookDirQuat });
	pECS->AddComponent<ScaleComponent>(playerEntity,			ScaleComponent{ .Scale = pPlayerDesc->Scale });
	pECS->AddComponent<VelocityComponent>(playerEntity,			VelocityComponent());
	pECS->AddComponent<TeamComponent>(playerEntity,				TeamComponent{ .TeamIndex = pPlayerDesc->TeamIndex });
	pECS->AddComponent<PacketComponent<PacketPlayerAction>>(playerEntity, { });
	pECS->AddComponent<PacketComponent<PacketPlayerActionResponse>>(playerEntity, { });

	const CharacterColliderCreateInfo colliderInfo =
	{
		.Entity			= playerEntity,
		.Position		= pECS->GetComponent<PositionComponent>(playerEntity),
		.Rotation		= pECS->GetComponent<RotationComponent>(playerEntity),
		.CollisionGroup	= FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
		.CollisionMask	= (uint32)FCollisionGroup::COLLISION_GROUP_STATIC |
						 (uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER |
						 (uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG |
						 (uint32)FCollisionGroup::COLLISION_GROUP_DYNAMIC,
		.EntityID		= playerEntity
	};

	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	CharacterColliderComponent characterColliderComponent = pPhysicsSystem->CreateCharacterCapsule(
		colliderInfo,
		std::max(0.0f, PLAYER_CAPSULE_HEIGHT - 2.0f * PLAYER_CAPSULE_RADIUS),
		PLAYER_CAPSULE_RADIUS);

	pECS->AddComponent<CharacterColliderComponent>(playerEntity, characterColliderComponent);

	Entity weaponEntity = pECS->CreateEntity();
	pECS->AddComponent<WeaponComponent>(weaponEntity, { .WeaponOwner = playerEntity });
	pECS->AddComponent<PacketComponent<PacketWeaponFired>>(weaponEntity, { });
	pECS->AddComponent<OffsetComponent>(weaponEntity, OffsetComponent{ .Offset = pPlayerDesc->Scale * glm::vec3(0.5f, 1.5f, -0.2f) });
	pECS->AddComponent<PositionComponent>(weaponEntity, PositionComponent{ .Position = pPlayerDesc->Position });
	pECS->AddComponent<RotationComponent>(weaponEntity, RotationComponent{ .Quaternion = lookDirQuat });

	ChildComponent childComp;
	childComp.AddChild(weaponEntity, "weapon");

	int32 playerNetworkUID;
	int32 weaponNetworkUID;
	if (!MultiplayerUtils::IsServer())
	{
		pECS->AddComponent<ParticleEmitterComponent>(weaponEntity, ParticleEmitterComponent
			{
				.Active = false,
				.OneTime = true,
				.Explosive = 1.0f,
				.ParticleCount = 64,
				.EmitterShape = EEmitterShape::CONE,
				.Angle = 15.f,
				.VelocityRandomness = 0.5f,
				.Velocity = 10.0,
				.Acceleration = 0.0,
				.Gravity = -4.f,
				.LifeTime = 2.0f,
				.RadiusRandomness = 0.5f,
				.BeginRadius = 0.1f,
				.FrictionFactor = 0.f,
				.Bounciness = 0.f,
				.TileIndex = 14,
				.AnimationCount = 1,
				.FirstAnimationIndex = 16,
				.Color = glm::vec4(TeamHelper::GetTeamColor(pPlayerDesc->TeamIndex), 1.0f),
			}
		);

		playerNetworkUID = pPlayerDesc->PlayerNetworkUID;
		weaponNetworkUID = pPlayerDesc->WeaponNetworkUID;


		AnimationComponent animationComponent = {};
		animationComponent.Pose.pSkeleton = ResourceManager::GetMesh(s_PlayerMeshGUID)->pSkeleton;

		AnimationGraph* pAnimationGraph = DBG_NEW AnimationGraph();
		pAnimationGraph->AddState(DBG_NEW AnimationState("Idle", s_PlayerIdleGUIDs[0]));

#ifndef LAMBDA_DEBUG
		pAnimationGraph->AddState(DBG_NEW AnimationState("Running", s_PlayerRunGUIDs[0]));
		pAnimationGraph->AddState(DBG_NEW AnimationState("Run Backward", s_PlayerRunBackwardGUIDs[0]));
		pAnimationGraph->AddState(DBG_NEW AnimationState("Strafe Right", s_PlayerStrafeRightGUIDs[0]));
		pAnimationGraph->AddState(DBG_NEW AnimationState("Strafe Left", s_PlayerStrafeLeftGUIDs[0]));

		{
			AnimationState* pAnimationState = DBG_NEW AnimationState("Running & Strafe Left");
			ClipNode* pRunning		= pAnimationState->CreateClipNode(s_PlayerRunMirroredGUIDs[0]);
			ClipNode* pStrafeLeft	= pAnimationState->CreateClipNode(s_PlayerStrafeLeftGUIDs[0]);
			BlendNode* pBlendNode	= pAnimationState->CreateBlendNode(pStrafeLeft, pRunning, BlendInfo(0.5f));
			pAnimationState->SetOutputNode(pBlendNode);
			pAnimationGraph->AddState(pAnimationState);
		}

		{
			AnimationState* pAnimationState = DBG_NEW AnimationState("Running & Strafe Right");
			ClipNode* pRunning		= pAnimationState->CreateClipNode(s_PlayerRunGUIDs[0]);
			ClipNode* pStrafeRight	= pAnimationState->CreateClipNode(s_PlayerStrafeRightGUIDs[0]);
			BlendNode* pBlendNode	= pAnimationState->CreateBlendNode(pStrafeRight, pRunning, BlendInfo(0.5f));
			pAnimationState->SetOutputNode(pBlendNode);
			pAnimationGraph->AddState(pAnimationState);
		}

		{
			AnimationState* pAnimationState = DBG_NEW AnimationState("Run Backward & Strafe Left");
			ClipNode* pRunningBackward	= pAnimationState->CreateClipNode(s_PlayerRunBackwardMirroredGUIDs[0]);
			ClipNode* pStrafeLeft		= pAnimationState->CreateClipNode(s_PlayerStrafeLeftGUIDs[0]);
			BlendNode* pBlendNode		= pAnimationState->CreateBlendNode(pStrafeLeft, pRunningBackward, BlendInfo(0.5f));
			pAnimationState->SetOutputNode(pBlendNode);
			pAnimationGraph->AddState(pAnimationState);
		}

		{
			AnimationState* pAnimationState = DBG_NEW AnimationState("Run Backward & Strafe Right");
			ClipNode* pRunningBackward	= pAnimationState->CreateClipNode(s_PlayerRunBackwardGUIDs[0]);
			ClipNode* pStrafeRight		= pAnimationState->CreateClipNode(s_PlayerStrafeRightGUIDs[0]);
			BlendNode* pBlendNode		= pAnimationState->CreateBlendNode(pStrafeRight, pRunningBackward, BlendInfo(0.5f));
			pAnimationState->SetOutputNode(pBlendNode);
			pAnimationGraph->AddState(pAnimationState);
		}

		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Idle", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Right", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Strafe Left", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Left", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Run Backward & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Running & Strafe Right", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Left", "Run Backward & Strafe Right"));

		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Idle"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Running"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Run Backward"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Running & Strafe Left"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Running & Strafe Right"));
		pAnimationGraph->AddTransition(DBG_NEW Transition("Run Backward & Strafe Right", "Run Backward & Strafe Left"));
#endif
		animationComponent.pGraph = pAnimationGraph;

		pAnimationGraph->TransitionToState("Idle");

		pECS->AddComponent<AnimationComponent>(playerEntity, animationComponent);
		pECS->AddComponent<MeshComponent>(playerEntity, 
			MeshComponent
			{
				.MeshGUID		= s_PlayerMeshGUID, 
				.MaterialGUID	= TeamHelper::GetTeamColorMaterialGUID(pPlayerDesc->TeamIndex)
			});

		pECS->AddComponent<MeshPaintComponent>(playerEntity, MeshPaint::CreateComponent(playerEntity, "PlayerUnwrappedTexture", 512, 512));

		if (!pPlayerDesc->IsLocal)
		{
			pECS->AddComponent<PlayerForeignComponent>(playerEntity, PlayerForeignComponent());
		}
		else
		{
			if (pPlayerDesc->pCameraDesc == nullptr)
			{
				pECS->RemoveEntity(playerEntity);
				LOG_ERROR("[LevelObjectCreator]: Local Player must have a camera description");
				return false;
			}

			pECS->AddComponent<PlayerLocalComponent>(playerEntity, PlayerLocalComponent());
			EntityMaskManager::AddExtensionToEntity(playerEntity, PlayerLocalComponent::Type(), nullptr);

			//Create Camera Entity
			Entity cameraEntity = pECS->CreateEntity();
			childEntities.PushBack(cameraEntity);
			childComp.AddChild(cameraEntity, "camera");

			//Todo: Better implementation for this somehow maybe?
			const Mesh* pMesh = ResourceManager::GetMesh(s_PlayerMeshGUID);
			OffsetComponent offsetComponent = { .Offset = pPlayerDesc->Scale * glm::vec3(0.0f, 0.95f * pMesh->BoundingBox.Dimensions.y, 0.0f) };

			pECS->AddComponent<OffsetComponent>(cameraEntity, offsetComponent);
			pECS->AddComponent<PositionComponent>(cameraEntity, PositionComponent{ .Position = pPlayerDesc->Position + offsetComponent.Offset });
			pECS->AddComponent<ScaleComponent>(cameraEntity, ScaleComponent{ .Scale = {1.0f, 1.0f, 1.0f} });
			pECS->AddComponent<RotationComponent>(cameraEntity, RotationComponent{ .Quaternion = lookDirQuat });
			pECS->AddComponent<ListenerComponent>(cameraEntity, { AudioAPI::GetDevice()->CreateAudioListener() });

			const ViewProjectionMatricesComponent viewProjComp =
			{
				.Projection = glm::perspective(
					glm::radians(pPlayerDesc->pCameraDesc->FOVDegrees),
					pPlayerDesc->pCameraDesc->Width / pPlayerDesc->pCameraDesc->Height,
					pPlayerDesc->pCameraDesc->NearPlane,
					pPlayerDesc->pCameraDesc->FarPlane),

				.View = glm::lookAt(
					pPlayerDesc->Position,
					pPlayerDesc->Position + pPlayerDesc->Forward,
					g_DefaultUp)
			};
			pECS->AddComponent<ViewProjectionMatricesComponent>(cameraEntity, viewProjComp);

			const CameraComponent cameraComp =
			{
				.NearPlane	= pPlayerDesc->pCameraDesc->NearPlane,
				.FarPlane	= pPlayerDesc->pCameraDesc->FarPlane,
				.FOV		= pPlayerDesc->pCameraDesc->FOVDegrees
			};
			pECS->AddComponent<CameraComponent>(cameraEntity, cameraComp);
			pECS->AddComponent<ParentComponent>(cameraEntity, ParentComponent{ .Parent = playerEntity, .Attached = true });
		}
	}
	else
	{
		playerNetworkUID = (int32)playerEntity;
		weaponNetworkUID = (int32)weaponEntity;
	}

	pECS->AddComponent<NetworkComponent>(playerEntity, { playerNetworkUID });
	pECS->AddComponent<ChildComponent>(playerEntity, childComp);
	pECS->AddComponent<HealthComponent>(playerEntity, HealthComponent());
	pECS->AddComponent<PacketComponent<PacketHealthChanged>>(playerEntity, {});

	pECS->AddComponent<NetworkComponent>(weaponEntity, { weaponNetworkUID });
	D_LOG_INFO("Created Player with EntityID %d and NetworkID %d", playerEntity, playerNetworkUID);
	D_LOG_INFO("Created Weapon with EntityID %d and NetworkID %d", weaponEntity, weaponNetworkUID);

	return true;
}

bool LevelObjectCreator::CreateProjectile(
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(createdChildEntities);

	if (pData == nullptr)
	{
		return false;
	}

	const CreateProjectileDesc& desc = *reinterpret_cast<const CreateProjectileDesc*>(pData);

	// Create a projectile entity
	ECSCore* pECS = ECSCore::GetInstance();
	const Entity projectileEntity = pECS->CreateEntity();
	createdEntities.PushBack(projectileEntity);

	const VelocityComponent velocityComponent = { desc.InitalVelocity };
	pECS->AddComponent<VelocityComponent>(projectileEntity, velocityComponent);

	ProjectileComponent projectileComp;
	projectileComp.AmmoType	= desc.AmmoType;
	projectileComp.Owner	= desc.WeaponOwner;
	pECS->AddComponent<ProjectileComponent>(projectileEntity, projectileComp);
	pECS->AddComponent<TeamComponent>(projectileEntity, { static_cast<uint8>(desc.TeamIndex) });

	if (!MultiplayerUtils::IsServer())
	{
		pECS->AddComponent<MeshComponent>(projectileEntity, desc.MeshComponent );
	}

	const DynamicCollisionCreateInfo collisionInfo =
	{
		/* Entity */	 		projectileEntity,
		/* Detection Method */	ECollisionDetection::CONTINUOUS,
		/* Position */	 		pECS->AddComponent<PositionComponent>(projectileEntity, { true, desc.FirePosition }),
		/* Scale */				pECS->AddComponent<ScaleComponent>(projectileEntity, { true, glm::vec3(1.0f) }),
		/* Rotation */			pECS->AddComponent<RotationComponent>(projectileEntity, { true, desc.FireDirection }),
		{
			{
				/* Shape Type */		EShapeType::SIMULATION,
				/* Geometry Type */		EGeometryType::SPHERE,
				/* Geometry Params */	{ .Radius = 0.3f },
				/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_DYNAMIC,
				/* CollisionMask */		(uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER |
										(uint32)FCollisionGroup::COLLISION_GROUP_STATIC,
				/* EntityID*/			desc.WeaponOwner,
				/* CallbackFunction */	desc.Callback,
			},
		},
		/* Velocity */			velocityComponent
	};

	const DynamicCollisionComponent projectileCollisionComp = PhysicsSystem::GetInstance()->CreateDynamicActor(collisionInfo);
	pECS->AddComponent<DynamicCollisionComponent>(projectileEntity, projectileCollisionComp);

	return true;
}
