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
#include "Teams/TeamHelper.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/ClientRemoteBase.h"
#include "Networking/API/BinaryEncoder.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Math/Math.h"
#include "Math/Random.h"

#include "Resources/ResourceManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Rendering/EntityMaskManager.h"

#include "Multiplayer/PacketType.h"

#include "Physics/CollisionGroups.h"
bool LevelObjectCreator::Init()
{
	using namespace LambdaEngine;

	//Register Create Special Object by Prefix Functions
	{
		//Spawnpoint
		{
			SpecialObjectOnLoadDesc specialObjectDesc =
			{
				.Prefix = "SO_PLAYER_SPAWN"
			};

			s_SpecialObjectOnLoadDescriptions.PushBack(specialObjectDesc);
			s_SpecialObjectByPrefixCreateFunctions[specialObjectDesc.Prefix] = &LevelObjectCreator::CreatePlayerSpawn;
		}

		//Spawnpoint
		{
			SpecialObjectOnLoadDesc specialObjectDesc =
			{
				.Prefix = "SO_FLAG_SPAWN"
			};

			s_SpecialObjectOnLoadDescriptions.PushBack(specialObjectDesc);
			s_SpecialObjectByPrefixCreateFunctions[specialObjectDesc.Prefix] = &LevelObjectCreator::CreateFlagSpawn;
		}
	}

	//Register Create Special Object by Type Functions
	{
		s_SpecialObjectByTypeCreateFunctions[ESpecialObjectType::SPECIAL_OBJECT_TYPE_FLAG_]		= &LevelObjectCreator::CreateFlag;
		s_SpecialObjectByTypeCreateFunctions[ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER]	= &LevelObjectCreator::CreatePlayer;
	}

	//Load Object Meshes & Materials
	{
		//Flag
		{
			s_FlagMeshGUID		= ResourceManager::LoadMeshFromFile("bunny.obj");

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
	const CollisionCreateInfo collisionCreateInfo =
	{
		.Entity			= entity,
		.Position		= pECS->AddComponent<PositionComponent>(entity, { true, pMesh->DefaultPosition + translation }),
		.Scale			= pECS->AddComponent<ScaleComponent>(entity,	{ true, pMesh->DefaultScale }),
		.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, pMesh->DefaultRotation }),
		.Mesh			= pECS->AddComponent<MeshComponent>(entity,		meshComponent),
		.ShapeType		= EShapeType::SIMULATION,
		.CollisionGroup = FCollisionGroup::COLLISION_GROUP_STATIC,
		.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
	};

	StaticCollisionComponent staticCollisionComponent = pPhysicsSystem->CreateStaticCollisionMesh(collisionCreateInfo);
	pECS->AddComponent<StaticCollisionComponent>(entity, staticCollisionComponent);
	return entity;
}

ESpecialObjectType LevelObjectCreator::CreateSpecialObjectFromPrefix(const LambdaEngine::SpecialObjectOnLoad& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	auto createFuncIt = s_SpecialObjectByPrefixCreateFunctions.find(specialObject.Prefix);

	if (createFuncIt != s_SpecialObjectByPrefixCreateFunctions.end())
	{
		return createFuncIt->second(specialObject, createdEntities, translation);
	}
	else
	{
		LOG_ERROR("[LevelObjectCreator]: Failed to create special object %s with prefix %s, no create function could be found", specialObject.Name.c_str(), specialObject.Prefix.c_str());
		return ESpecialObjectType::SPECIAL_OBJECT_TYPE_NONE;
	}
}

bool LevelObjectCreator::CreateSpecialObjectOfType(
	ESpecialObjectType specialObjectType,
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities,
	LambdaEngine::TArray<uint64>& saltUIDs)
{
	auto createFuncIt = s_SpecialObjectByTypeCreateFunctions.find(specialObjectType);

	if (createFuncIt != s_SpecialObjectByTypeCreateFunctions.end())
	{
		return createFuncIt->second(pData, createdEntities, createdChildEntities, saltUIDs);
	}
	else
	{
		LOG_ERROR("[LevelObjectCreator]: Failed to create special object, no create function could be found");
		return false;
	}
}

ESpecialObjectType LevelObjectCreator::CreatePlayerSpawn(const LambdaEngine::SpecialObjectOnLoad& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	UNREFERENCED_VARIABLE(specialObject);
	UNREFERENCED_VARIABLE(createdEntities);
	UNREFERENCED_VARIABLE(translation);

	LOG_WARNING("[LevelObjectCreator]: Spawnpoint not implemented!");
	return ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER_SPAWN;
}

ESpecialObjectType LevelObjectCreator::CreateFlagSpawn(const LambdaEngine::SpecialObjectOnLoad& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	return ESpecialObjectType::SPECIAL_OBJECT_TYPE_FLAG_SPAWN;
}

bool LevelObjectCreator::CreateFlag(
	const void* pData, 
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, 
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities, 
	LambdaEngine::TArray<uint64>& saltUIDs)
{
	if (pData == nullptr) return false;

	using namespace LambdaEngine;

	const CreateFlagDesc* pFlagDesc = reinterpret_cast<const CreateFlagDesc*>(pData);

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	Entity entity = pECS->CreateEntity();

	pECS->AddComponent<FlagComponent>(entity, FlagComponent());
	pECS->AddComponent<OffsetComponent>(entity, OffsetComponent{ .Offset = glm::vec3(1.0f) });
	pECS->AddComponent<ParentComponent>(entity, ParentComponent{ .Attached = false });

	//Network Stuff
	{
		pECS->AddComponent<PacketComponent<FlagEditedPacket>>(entity, {});
	}

	if (!MultiplayerUtils::IsServer())
	{
		pECS->AddComponent<NetworkComponent>(entity, { pFlagDesc->NetworkUID });
		MultiplayerUtils::RegisterEntity(entity, pFlagDesc->NetworkUID);
	}
	else
	{
		//Only the server checks collision with the flag
		const DynamicCollisionCreateInfo collisionCreateInfo =
		{
			/* Entity */	 		entity,
			/* Position */	 		pECS->AddComponent<PositionComponent>(entity,		{ true, pFlagDesc->Position }),
			/* Scale */				pECS->AddComponent<ScaleComponent>(entity,			{ true, pFlagDesc->Scale }),
			/* Rotation */			pECS->AddComponent<RotationComponent>(entity,		{ true, pFlagDesc->Rotation }),
			/* Mesh */				pECS->AddComponent<MeshComponent>(entity,			{ pFlagDesc->MeshGUID, }),
			/* Shape Type */		EShapeType::TRIGGER,
			/* CollisionGroup */	FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG,
			/* CollisionMask */		FLAG_DROPPED_COLLISION_MASK,
			/* CallbackFunction */	std::bind_front(&FlagSystemBase::OnPlayerFlagCollision, FlagSystemBase::GetInstance()),
			/* Velocity */			pECS->AddComponent<VelocityComponent>(entity,		{ glm::vec3(0.0f) })
		};
		DynamicCollisionComponent collisionComponent = pPhysicsSystem->CreateDynamicCollisionBox(collisionCreateInfo);
		collisionComponent.pActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		pECS->AddComponent<DynamicCollisionComponent>(entity, collisionComponent);

		pECS->AddComponent<NetworkComponent>(entity, { (int32)entity });
		MultiplayerUtils::RegisterEntity(entity, (int32)entity);
	}

	createdEntities.PushBack(entity);

	return true;
}

bool LevelObjectCreator::CreatePlayer(
	const void* pData,
	LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities,
	LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>>& createdChildEntities,
	LambdaEngine::TArray<uint64>& saltUIDs)
{
	if (pData == nullptr) return false;

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

	pECS->AddComponent<PacketComponent<PlayerAction>>(playerEntity, { });
	pECS->AddComponent<PacketComponent<PlayerActionResponse>>(playerEntity, { });

	const CharacterColliderCreateInfo colliderInfo =
	{
		.Entity			= playerEntity,
		.Position		= pECS->GetComponent<PositionComponent>(playerEntity),
		.Rotation		= pECS->GetComponent<RotationComponent>(playerEntity),
		.CollisionGroup	= FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER,
		.CollisionMask	= FCollisionGroup::COLLISION_GROUP_STATIC | FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER | FCrazyCanvasCollisionGroup::COLLISION_GROUP_FLAG
	};

	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	CharacterColliderComponent characterColliderComponent = pPhysicsSystem->CreateCharacterCapsule(colliderInfo, std::max(0.0f, PLAYER_CAPSULE_HEIGHT - 2.0f * PLAYER_CAPSULE_RADIUS), PLAYER_CAPSULE_RADIUS);
	pECS->AddComponent<CharacterColliderComponent>(playerEntity, characterColliderComponent);

	Entity weaponEntity = pECS->CreateEntity();
	pECS->AddComponent<WeaponComponent>(weaponEntity, { .WeaponOwner = playerEntity, });

	if (!MultiplayerUtils::IsServer())
	{
		//Todo: Set DrawArgs Mask here to avoid rendering local mesh
		pECS->AddComponent<MeshComponent>(playerEntity, MeshComponent{.MeshGUID = pPlayerDesc->MeshGUID, .MaterialGUID = TeamHelper::GetTeamColorMaterialGUID(pPlayerDesc->TeamIndex)});
		pECS->AddComponent<AnimationComponent>(playerEntity, pPlayerDesc->AnimationComponent);
		pECS->AddComponent<MeshPaintComponent>(playerEntity, MeshPaint::CreateComponent(playerEntity, "PlayerUnwrappedTexture", 512, 512));
		pECS->AddComponent<NetworkComponent>(playerEntity, { pPlayerDesc->NetworkUID });

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
		}

		MultiplayerUtils::RegisterEntity(playerEntity, pPlayerDesc->NetworkUID);
		saltUIDs.PushBack(pPlayerDesc->pClient->GetStatistics()->GetRemoteSalt());
	}
	else
	{
		pECS->AddComponent<NetworkComponent>(playerEntity, { (int32)playerEntity });

		saltUIDs.PushBack(pPlayerDesc->pClient->GetStatistics()->GetSalt());

		ClientRemoteBase* pClient = reinterpret_cast<ClientRemoteBase*>(pPlayerDesc->pClient);

		{
			NetworkSegment* pPacket = pClient->GetFreePacket(PacketType::CREATE_ENTITY);
			BinaryEncoder encoder = BinaryEncoder(pPacket);
			encoder.WriteBool(true);
			encoder.WriteInt32((int32)playerEntity);
			encoder.WriteVec3(pPlayerDesc->Position);
			encoder.WriteVec3(pPlayerDesc->Forward);
			encoder.WriteUInt32(pPlayerDesc->TeamIndex);

			//Todo: 2nd argument should not be nullptr if we want a little info
			pClient->SendReliable(pPacket, nullptr);
		}

		const ClientMap& clients = pClient->GetClients();

		for (auto& clientPair : clients)
		{
			if (clientPair.second != pClient)
			{
				//Send to everyone already connected
				NetworkSegment* pPacket = clientPair.second->GetFreePacket(PacketType::CREATE_ENTITY);
				BinaryEncoder encoder(pPacket);
				encoder.WriteBool(false);
				encoder.WriteInt32((int32)playerEntity);
				encoder.WriteVec3(pPlayerDesc->Position);
				encoder.WriteVec3(pPlayerDesc->Forward);
				encoder.WriteUInt32(pPlayerDesc->TeamIndex);
				clientPair.second->SendReliable(pPacket, nullptr);
			}
		}
	}


	D_LOG_INFO("Created Player with EntityID %d and NetworkID %d", playerEntity, pECS->GetComponent<NetworkComponent>(playerEntity).NetworkUID);
	return true;
}