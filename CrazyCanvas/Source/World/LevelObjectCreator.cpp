#include "World/LevelObjectCreator.h"
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

#include "ECS/Systems/Match/FlagSystemBase.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/Match/BaseComponent.h"
#include "Teams/TeamHelper.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Player/HealthComponent.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryEncoder.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Math/Math.h"
#include "Math/Random.h"

#include "Resources/ResourceManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Rendering/EntityMaskManager.h"

#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/Packet/PlayerAction.h"
#include "Multiplayer/Packet/PlayerActionResponse.h"

#include "Physics/CollisionGroups.h"

bool LevelObjectCreator::Init()
{
	using namespace LambdaEngine;

	//Register Create Special Object by Prefix Functions
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

		//Base
		{
			LevelObjectOnLoadDesc levelObjectDesc =
			{
				.Prefix = "SO_BASE_"
			};

			s_LevelObjectOnLoadDescriptions.PushBack(levelObjectDesc);
			s_LevelObjectByPrefixCreateFunctions[levelObjectDesc.Prefix] = &LevelObjectCreator::CreateBase;
		}
	}

	//Register Create Special Object by Type Functions
	{
		s_LevelObjectByTypeCreateFunctions[ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG]	= &LevelObjectCreator::CreateFlag;
		s_LevelObjectByTypeCreateFunctions[ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER]	= &LevelObjectCreator::CreatePlayer;
	}

	//Load Object Meshes & Materials
	{
		//Flag
		{
			s_FlagMeshGUID		= ResourceManager::LoadMeshFromFile("gun.obj");

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
	}

	return true;
}

LambdaEngine::Entity LevelObjectCreator::CreateDirectionalLight(const LambdaEngine::LoadedDirectionalLight& directionalLight, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	Entity entity = UINT32_MAX;

	if (!MultiplayerUtils::IsServer())
	{
		ECSCore* pECS = ECSCore::GetInstance();

		DirectionalLightComponent directionalLightComponent =
		{
			.ColorIntensity = directionalLight.ColorIntensity
		};

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

	const Mesh* pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID);

	ECSCore* pECS					= ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem	= PhysicsSystem::GetInstance();

	Entity entity = pECS->CreateEntity();
	pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "GeometryUnwrappedTexture", 4096, 4096));
	pECS->AddComponent<MeshComponent>(entity, meshComponent);
	const CollisionCreateInfo collisionCreateInfo =
	{
		.Entity			= entity,
		.Position		= pECS->AddComponent<PositionComponent>(entity, { true, pMesh->DefaultPosition + translation }),
		.Scale			= pECS->AddComponent<ScaleComponent>(entity,	{ true, pMesh->DefaultScale }),
		.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, pMesh->DefaultRotation }),
		.ShapeType		= EShapeType::SIMULATION,
		.CollisionGroup = FCollisionGroup::COLLISION_GROUP_STATIC,
		.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
	};

	StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticCollisionMesh(collisionCreateInfo, ResourceManager::GetMesh(meshComponent.MeshGUID));
	pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	return entity;
}

ELevelObjectType LevelObjectCreator::CreateLevelObjectFromPrefix(const LambdaEngine::LevelObjectOnLoad& levelObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
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
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities,
	LambdaEngine::TArray<uint64>& saltUIDs)
{
	auto createFuncIt = s_LevelObjectByTypeCreateFunctions.find(levelObjectType);

	if (createFuncIt != s_LevelObjectByTypeCreateFunctions.end())
	{
		return createFuncIt->second(pData, createdEntities, createdChildEntities, saltUIDs);
	}
	else
	{
		LOG_ERROR("[LevelObjectCreator]: Failed to create special object, no create function could be found");
		return false;
	}
}

ELevelObjectType LevelObjectCreator::CreatePlayerSpawn(const LambdaEngine::LevelObjectOnLoad& levelObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
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

	if (!levelObject.MeshComponents.IsEmpty())
	{
		const MeshComponent& meshComponent = levelObject.MeshComponents[0];

		pECS->AddComponent<MeshComponent>(entity, meshComponent);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "GeometryUnwrappedTexture", 512, 512));

		const Mesh* pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID);
		PhysicsSystem* pPhysicsSystem	= PhysicsSystem::GetInstance();

		const CollisionCreateInfo collisionCreateInfo =
		{
			.Entity			= entity,
			.Position		= positionComponent,
			.Scale			= scaleComponent,
			.Rotation		= rotationComponent,
			.ShapeType		= EShapeType::SIMULATION,
			.CollisionGroup = FCollisionGroup::COLLISION_GROUP_STATIC,
			.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
		};

		StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticCollisionMesh(collisionCreateInfo, pMesh);
		pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	}

	D_LOG_INFO("Created Player Spawn with EntityID %d", entity);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER_SPAWN;
}

ELevelObjectType LevelObjectCreator::CreateFlagSpawn(const LambdaEngine::LevelObjectOnLoad& levelObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Entity entity = pECS->CreateEntity();

	pECS->AddComponent<FlagSpawnComponent>(entity, { 1.0f });
	pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });

	createdEntities.PushBack(entity);

	D_LOG_INFO("Created Flag Spawn with EntityID %d", entity);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_FLAG_SPAWN;
}

ELevelObjectType LevelObjectCreator::CreateBase(const LambdaEngine::LevelObjectOnLoad& levelObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
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

	pECS->AddComponent<BaseComponent>(entity, { });
	const PositionComponent& positionComponent = pECS->AddComponent<PositionComponent>(entity, { true, levelObject.DefaultPosition + translation });
	const ScaleComponent& scaleComponent = pECS->AddComponent<ScaleComponent>(entity, { true, levelObject.DefaultScale });
	const RotationComponent& rotationComponent = pECS->AddComponent<RotationComponent>(entity, { true, levelObject.DefaultRotation });

	//Only the server checks collision with the flag
	const CollisionCreateInfo collisionCreateInfo =
	{
		/* Entity */	 		entity,
		/* Position */	 		positionComponent,
		/* Scale */				scaleComponent,
		/* Rotation */			rotationComponent,
		/* Shape Type */		EShapeType::TRIGGER,
		/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_BASE,
		/* CollisionMask */		FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
	};

	StaticCollisionComponent collisionComponent = pPhysicsSystem->CreateStaticCollisionBox(collisionCreateInfo, scaleComponent.Scale * boundingBox.Dimensions);
	pECS->AddComponent<StaticCollisionComponent>(entity, collisionComponent);

	createdEntities.PushBack(entity);

	D_LOG_INFO("Created Base with EntityID %d", entity);
	return ELevelObjectType::LEVEL_OBJECT_TYPE_BASE;
}

bool LevelObjectCreator::CreateFlag(
	const void* pData, 
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, 
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities, 
	LambdaEngine::TArray<uint64>& saltUIDs)
{
	UNREFERENCED_VARIABLE(createdChildEntities);
	UNREFERENCED_VARIABLE(saltUIDs);

	if (pData == nullptr) return false;

	using namespace LambdaEngine;

	const CreateFlagDesc* pFlagDesc = reinterpret_cast<const CreateFlagDesc*>(pData);

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	Entity entity = pECS->CreateEntity();

	Timestamp pickupCooldown = 1000 * 1000 * 1000;
	FlagComponent flagComponent{ EngineLoop::GetTimeSinceStart() + pickupCooldown, pickupCooldown };
	PositionComponent positionComponent{ true, pFlagDesc->Position };
	ScaleComponent scaleComponent{ true, pFlagDesc->Scale };
	RotationComponent rotationComponent{ true, pFlagDesc->Rotation };
	MeshComponent meshComponent{ s_FlagMeshGUID, s_FlagMaterialGUID };

	pECS->AddComponent<FlagComponent>(entity,		flagComponent);
	pECS->AddComponent<PositionComponent>(entity,	positionComponent);
	pECS->AddComponent<ScaleComponent>(entity,		scaleComponent);
	pECS->AddComponent<RotationComponent>(entity,	rotationComponent);
	pECS->AddComponent<MeshComponent>(entity,		meshComponent);

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

	pECS->AddComponent<ParentComponent>(entity,	parentComponent);
	pECS->AddComponent<OffsetComponent>(entity,	offsetComponent);

	//Network Stuff
	{
		pECS->AddComponent<PacketComponent<FlagEditedPacket>>(entity, {});
	}

	int32 networkUID;
	if (!MultiplayerUtils::IsServer())
	{
		networkUID = pFlagDesc->NetworkUID;
	}
	else
	{
		//Only the server checks collision with the flag
		const DynamicCollisionCreateInfo collisionCreateInfo =
		{
			/* Entity */	 		entity,
			/* Position */	 		positionComponent,
			/* Scale */				scaleComponent,
			/* Rotation */			rotationComponent,
			/* Shape Type */		EShapeType::TRIGGER,
			/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
			/* CollisionMask */		FLAG_DROPPED_COLLISION_MASK,
			/* CallbackFunction */	std::bind_front(&FlagSystemBase::OnPlayerFlagCollision, FlagSystemBase::GetInstance()),
			/* Velocity */			pECS->AddComponent<VelocityComponent>(entity, { glm::vec3(0.0f) })
		};
		DynamicCollisionComponent collisionComponent = pPhysicsSystem->CreateDynamicCollisionBox(collisionCreateInfo, scaleComponent.Scale * ResourceManager::GetMesh(meshComponent.MeshGUID)->BoundingBox.Dimensions);
		collisionComponent.pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

		//Add second shape (Non-Trigger) to allow Base-Flag Collisions

		pECS->AddComponent<DynamicCollisionComponent>(entity, collisionComponent);

		networkUID = (int32)entity;
	}

	pECS->AddComponent<NetworkComponent>(entity, { networkUID });
	//MultiplayerUtils::RegisterEntity(entity, networkUID);

	createdEntities.PushBack(entity);

	D_LOG_INFO("Created Flag with EntityID %d and NetworkID %d", entity, networkUID);
	return true;
}

bool LevelObjectCreator::CreatePlayer(
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities,
	LambdaEngine::TArray<uint64>& saltUIDs)
{
	if (pData == nullptr)
		return false;

	using namespace LambdaEngine;

	const CreatePlayerDesc* pPlayerDesc = reinterpret_cast<const CreatePlayerDesc*>(pData);

	ECSCore* pECS = ECSCore::GetInstance();
	Entity playerEntity = pECS->CreateEntity();
	createdEntities.PushBack(playerEntity);
	TArray<Entity>& childEntities = createdChildEntities.PushBack({});

	glm::quat lookDirQuat = glm::quatLookAt(pPlayerDesc->Forward, g_DefaultUp);

	pECS->AddComponent<PlayerBaseComponent>(playerEntity,		PlayerBaseComponent());
	pECS->AddComponent<PositionComponent>(playerEntity,			PositionComponent{ .Position = pPlayerDesc->Position });
	pECS->AddComponent<NetworkPositionComponent>(playerEntity,	NetworkPositionComponent{ .Position = pPlayerDesc->Position, .PositionLast = pPlayerDesc->Position, .TimestampStart = EngineLoop::GetTimeSinceStart(), .Duration = EngineLoop::GetFixedTimestep() });
	pECS->AddComponent<RotationComponent>(playerEntity,			RotationComponent{ .Quaternion = lookDirQuat });
	pECS->AddComponent<ScaleComponent>(playerEntity,			ScaleComponent{ .Scale = pPlayerDesc->Scale });
	pECS->AddComponent<VelocityComponent>(playerEntity,			VelocityComponent());
	pECS->AddComponent<TeamComponent>(playerEntity,				TeamComponent{ .TeamIndex = pPlayerDesc->TeamIndex });
	pECS->AddComponent<HealthComponent>(playerEntity,			HealthComponent());

	pECS->AddComponent<PacketComponent<PlayerAction>>(playerEntity, { });
	pECS->AddComponent<PacketComponent<PlayerActionResponse>>(playerEntity, { });

	const CharacterColliderCreateInfo colliderInfo =
	{
		.Entity			= playerEntity,
		.Position		= pECS->GetComponent<PositionComponent>(playerEntity),
		.Rotation		= pECS->GetComponent<RotationComponent>(playerEntity),
		.CollisionGroup	= FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
		.CollisionMask = (uint32)FCollisionGroup::COLLISION_GROUP_STATIC |
						 (uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER |
						 (uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG |
						 (uint32)FCollisionGroup::COLLISION_GROUP_DYNAMIC
	};

	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	CharacterColliderComponent characterColliderComponent = pPhysicsSystem->CreateCharacterCapsule(colliderInfo, std::max(0.0f, PLAYER_CAPSULE_HEIGHT - 2.0f * PLAYER_CAPSULE_RADIUS), PLAYER_CAPSULE_RADIUS);
	pECS->AddComponent<CharacterColliderComponent>(playerEntity, characterColliderComponent);

	Entity weaponEntity = pECS->CreateEntity();
	pECS->AddComponent<WeaponComponent>(weaponEntity, { .WeaponOwner = playerEntity, });

	int32 networkUID;
	if (!MultiplayerUtils::IsServer())
	{
		networkUID = pPlayerDesc->NetworkUID;

		//Todo: Set DrawArgs Mask here to avoid rendering local mesh
		pECS->AddComponent<MeshComponent>(playerEntity, MeshComponent{.MeshGUID = pPlayerDesc->MeshGUID, .MaterialGUID = TeamHelper::GetTeamColorMaterialGUID(pPlayerDesc->TeamIndex)});
		pECS->AddComponent<AnimationComponent>(playerEntity, pPlayerDesc->AnimationComponent);
		pECS->AddComponent<MeshPaintComponent>(playerEntity, MeshPaint::CreateComponent(playerEntity, "PlayerUnwrappedTexture", 512, 512));

		if (!pPlayerDesc->IsLocal)
		{
			pECS->AddComponent<PlayerForeignComponent>(playerEntity, PlayerForeignComponent());

			saltUIDs.PushBack(UINT64_MAX);
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

			//Todo: Better implementation for this somehow maybe?
			const Mesh* pMesh = ResourceManager::GetMesh(pPlayerDesc->MeshGUID);
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

			saltUIDs.PushBack(pPlayerDesc->pClient->GetStatistics()->GetRemoteSalt());
		}
	}
	else
	{
		networkUID = (int32)playerEntity;
		saltUIDs.PushBack(pPlayerDesc->pClient->GetStatistics()->GetSalt());
	}

	pECS->AddComponent<NetworkComponent>(playerEntity, { networkUID });

	D_LOG_INFO("Created Player with EntityID %d, NetworkID %d", playerEntity, networkUID);
	return true;
}