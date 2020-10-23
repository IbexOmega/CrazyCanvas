#include "World/LevelObjectCreator.h"

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

#include "Teams/TeamHelper.h"

#include "ECS/Components/Player/Weapon.h"

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

bool LevelObjectCreator::Init()
{
	using namespace LambdaEngine;

	//Register Create Special Object by Prefix Functions
	{
		//Spawnpoint
		{
			SpecialObjectOnLoadDesc specialObjectDesc =
			{
				.Prefix = "SO_SPAWN_"
			};

			s_SpecialObjectOnLoadDescriptions.PushBack(specialObjectDesc);
			s_SpecialObjectByPrefixCreateFunctions[specialObjectDesc.Prefix] = &LevelObjectCreator::CreateSpawnpoint;
		}

		//Spawnpoint
		{
			SpecialObjectOnLoadDesc specialObjectDesc =
			{
				.Prefix = "SO_FLAG_"
			};

			s_SpecialObjectOnLoadDescriptions.PushBack(specialObjectDesc);
			s_SpecialObjectByPrefixCreateFunctions[specialObjectDesc.Prefix] = &LevelObjectCreator::CreateFlag;
		}
	}

	//Register Create Special Object by Type Functions
	{
		s_SpecialObjectByTypeCreateFunctions[ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER] = &LevelObjectCreator::CreatePlayer;
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

	ECSCore* pECS					= ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem	= PhysicsSystem::GetInstance();

	Entity entity = pECS->CreateEntity();
	pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "GeometryUnwrappedTexture", 4096, 4096));
	const CollisionCreateInfo collisionCreateInfo =
	{
		.Entity			= entity,
		.Position		= pECS->AddComponent<PositionComponent>(entity, { true, translation }),
		.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, glm::vec3(1.0f) }),
		.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
		.Mesh			= pECS->AddComponent<MeshComponent>(entity, meshComponent),
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

ESpecialObjectType LevelObjectCreator::CreateSpawnpoint(const LambdaEngine::SpecialObjectOnLoad& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	UNREFERENCED_VARIABLE(specialObject);
	UNREFERENCED_VARIABLE(createdEntities);
	UNREFERENCED_VARIABLE(translation);

	LOG_WARNING("[LevelObjectCreator]: Spawnpoint not implemented!");
	return ESpecialObjectType::SPECIAL_OBJECT_TYPE_SPAWN_POINT;
}

ESpecialObjectType LevelObjectCreator::CreateFlag(const LambdaEngine::SpecialObjectOnLoad& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	UNREFERENCED_VARIABLE(specialObject);
	UNREFERENCED_VARIABLE(createdEntities);
	UNREFERENCED_VARIABLE(translation);

	LOG_WARNING("[LevelObjectCreator]: Create Flag not implemented!");
	return ESpecialObjectType::SPECIAL_OBJECT_TYPE_FLAG;
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
	pECS->AddComponent<RotationComponent>(playerEntity,			RotationComponent{ .Quaternion = lookDirQuat });
	pECS->AddComponent<NetworkPositionComponent>(playerEntity,	NetworkPositionComponent{ .Position = pPlayerDesc->Position, .PositionLast = pPlayerDesc->Position, .TimestampStart = EngineLoop::GetTimeSinceStart(), .Duration = EngineLoop::GetFixedTimestep() });
	pECS->AddComponent<ScaleComponent>(playerEntity,			ScaleComponent{ .Scale = pPlayerDesc->Scale });
	pECS->AddComponent<VelocityComponent>(playerEntity,			VelocityComponent());
	pECS->AddComponent<TeamComponent>(playerEntity,				TeamComponent{ .TeamIndex = pPlayerDesc->TeamIndex });

	const CharacterColliderCreateInfo colliderInfo =
	{
		.Entity			= playerEntity,
		.Position		= pECS->GetComponent<PositionComponent>(playerEntity),
		.Rotation		= pECS->GetComponent<RotationComponent>(playerEntity),
		.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_PLAYER,
		.CollisionMask	= FCollisionGroup::COLLISION_GROUP_STATIC | FCollisionGroup::COLLISION_GROUP_PLAYER
	};

	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();
	CharacterColliderComponent characterColliderComponent = pPhysicsSystem->CreateCharacterCapsule(colliderInfo, std::max(0.0f, PLAYER_CAPSULE_HEIGHT - 2.0f * PLAYER_CAPSULE_RADIUS), PLAYER_CAPSULE_RADIUS);
	pECS->AddComponent<CharacterColliderComponent>(playerEntity, characterColliderComponent);
	pECS->AddComponent<NetworkComponent>(playerEntity, { (int32)playerEntity });

	Entity weaponEntity = pECS->CreateEntity();
	pECS->AddComponent<OffsetComponent>(weaponEntity, OffsetComponent{ .Offset = pPlayerDesc->Scale * glm::vec3(0.0, 1.8f, 0.0) });
	pECS->AddComponent<WeaponComponent>(weaponEntity, { .WeaponOwner = playerEntity, });
	pECS->AddComponent<PositionComponent>(weaponEntity, PositionComponent{ .Position = pPlayerDesc->Position });
	pECS->AddComponent<RotationComponent>(weaponEntity, RotationComponent{ .Quaternion = lookDirQuat });
	pECS->AddComponent<ParticleEmitterComponent>(weaponEntity, ParticleEmitterComponent{
		.Active = false,
		.OneTime = true,
		.Explosive = 1.0f,
		.ParticleCount = 32,
		.EmitterShape = EEmitterShape::CONE,
		.Angle = 15.f,
		.Velocity = 4.0,
		.Acceleration = 0.0,
		.Gravity = -4.f,
		.LifeTime = 2.0f,
		.BeginRadius = 0.1f,
		.TileIndex = 14,
		.AnimationCount = 1,
		.FirstAnimationIndex = 16,
		.Color = glm::vec4(0.0f, 0.0f, 0.5f, 1.0f),
		}
	);

	if (!MultiplayerUtils::IsServer())
	{
		//Todo: Set DrawArgs Mask here to avoid rendering local mesh
		pECS->AddComponent<MeshComponent>(playerEntity, MeshComponent{.MeshGUID = pPlayerDesc->MeshGUID, .MaterialGUID = TeamHelper::GetTeamColorMaterialGUID(pPlayerDesc->TeamIndex)});
		pECS->AddComponent<AnimationComponent>(playerEntity, pPlayerDesc->AnimationComponent);
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
		saltUIDs.PushBack(pPlayerDesc->pClient->GetStatistics()->GetSalt());

		ClientRemoteBase* pClient = reinterpret_cast<ClientRemoteBase*>(pPlayerDesc->pClient);

		{
			NetworkSegment* pPacket = pClient->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
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
				NetworkSegment* pPacket = clientPair.second->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
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


	D_LOG_INFO("Created Player");
	return true;
}