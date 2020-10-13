#include "World/LevelObjectCreator.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

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
	pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "GeometryUnwrappedTexture", 512, 512));
	const CollisionInfo collisionCreateInfo = 
	{
		.Entity			= entity,
		.Position		= pECS->AddComponent<PositionComponent>(entity, { true, translation }),
		.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, glm::vec3(1.0f) }),
		.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
		.Mesh			= pECS->AddComponent<MeshComponent>(entity, meshComponent),
		.CollisionGroup = FCollisionGroup::COLLISION_GROUP_STATIC,
		.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
	};

	pPhysicsSystem->CreateStaticCollisionMesh(collisionCreateInfo);
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
		LOG_ERROR("[LevelObjectCreator]: Failed to create special object %s with prefix %s, no create function could be found", specialObject.Name, specialObject.Prefix);
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
	pECS->AddComponent<NetworkPositionComponent>(playerEntity,	NetworkPositionComponent{ .Position = pPlayerDesc->Position, .PositionLast = pPlayerDesc->Position, .TimestampStart = EngineLoop::GetTimeSinceStart(), .Duration = EngineLoop::GetFixedTimestep() });
	pECS->AddComponent<RotationComponent>(playerEntity,			RotationComponent{ .Quaternion = lookDirQuat });
	pECS->AddComponent<ScaleComponent>(playerEntity,			ScaleComponent{ .Scale = pPlayerDesc->Scale });
	pECS->AddComponent<VelocityComponent>(playerEntity,			VelocityComponent());

	const CharacterColliderInfo colliderInfo = 
	{
		.Entity			= playerEntity,
		.Position		= pECS->GetComponent<PositionComponent>(playerEntity),
		.Rotation		= pECS->GetComponent<RotationComponent>(playerEntity),
		.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_PLAYER,
		.CollisionMask	= FCollisionGroup::COLLISION_GROUP_STATIC | FCollisionGroup::COLLISION_GROUP_PLAYER
	};

	CharacterColliderComponent characterColliderComponent;
	PhysicsSystem::GetInstance()->CreateCharacterCapsule(colliderInfo, std::max(0.0f, PLAYER_CAPSULE_HEIGHT - 2.0f * PLAYER_CAPSULE_RADIUS), PLAYER_CAPSULE_RADIUS, characterColliderComponent);
	pECS->AddComponent<CharacterColliderComponent>(playerEntity, characterColliderComponent);
	pECS->AddComponent<NetworkComponent>(playerEntity, { (int32)playerEntity });

	if (!MultiplayerUtils::IsServer())
	{
		//Todo: Set DrawArgs Mask here to avoid rendering local mesh
		pECS->AddComponent<MeshComponent>(playerEntity, pPlayerDesc->MeshComponent);
		
		pECS->AddComponent<MeshPaintComponent>(playerEntity, MeshPaint::CreateComponent(playerEntity, "PlayerUnwrappedTexture", 512, 512));

		if (!pPlayerDesc->IsLocal)
		{
			pECS->AddComponent<PlayerForeignComponent>(playerEntity, PlayerForeignComponent());

			AnimationComponent animationCopy = pPlayerDesc->AnimationComponent;
			animationCopy.Graph.GetCurrentState().SetPlaybackSpeed(2.0f);
			pECS->AddComponent<AnimationComponent>(playerEntity, animationCopy);
		}
		else
		{
			if (pPlayerDesc->pCameraDesc == nullptr)
			{
				pECS->RemoveEntity(playerEntity);
				LOG_ERROR("[LevelObjectCreator]: Local Player must have a camera description");
				return false;
			}

			pECS->AddComponent<AnimationComponent>(playerEntity, pPlayerDesc->AnimationComponent);
			pECS->AddComponent<PlayerLocalComponent>(playerEntity, PlayerLocalComponent());
			EntityMaskManager::AddExtensionToEntity(playerEntity, PlayerLocalComponent::Type(), nullptr);

			//Create Camera Entity
			Entity cameraEntity = pECS->CreateEntity();
			childEntities.PushBack(cameraEntity);

			//Todo: Better implementation for this somehow maybe?
			const Mesh* pMesh = ResourceManager::GetMesh(pPlayerDesc->MeshComponent.MeshGUID);

			OffsetComponent offsetComponent = { .Offset = pPlayerDesc->Scale * glm::vec3(0.0f, pMesh->BoundingBox.HalfExtent.y, 0.0f) };

			pECS->AddComponent<OffsetComponent>(cameraEntity, offsetComponent);
			pECS->AddComponent<PositionComponent>(cameraEntity, PositionComponent{ .Position = pPlayerDesc->Position + offsetComponent.Offset });
			pECS->AddComponent<ScaleComponent>(cameraEntity, ScaleComponent{ .Scale = {1.0f, 1.0f, 1.0f} });
			pECS->AddComponent<RotationComponent>(cameraEntity, RotationComponent{ .Quaternion = lookDirQuat });

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

		NetworkSegment* pPacket = pClient->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
		BinaryEncoder encoder = BinaryEncoder(pPacket);
		encoder.WriteBool(true);
		encoder.WriteInt32((int32)playerEntity);
		encoder.WriteVec3(pPlayerDesc->Position);

		//Todo: 2nd argument should not be nullptr if we want a little info
		pClient->SendReliable(pPacket, nullptr);

		const auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		const ClientMap& clients = pClient->GetClients();

		for (auto& clientPair : clients)
		{
			if (clientPair.second != pClient)
			{
				//Send to everyone already connected
				NetworkSegment* pPacket2 = clientPair.second->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
				BinaryEncoder encoder2(pPacket2);
				encoder2.WriteBool(false);
				encoder2.WriteInt32((int32)playerEntity);
				encoder2.WriteVec3(pPlayerDesc->Position);
				clientPair.second->SendReliable(pPacket2, nullptr);
			}
		}
	}


	D_LOG_INFO("Created Player");
	return true;
}