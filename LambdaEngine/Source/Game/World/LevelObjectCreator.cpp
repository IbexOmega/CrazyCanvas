#include "Game/World/LevelObjectCreator.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Math/Math.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	bool LevelObjectCreator::Init(bool clientSide)
	{
		using namespace LambdaEngine;

		m_ClientSide = clientSide;

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

		if (m_ClientSide)
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

		if (m_ClientSide)
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
		const StaticCollisionInfo collisionCreateInfo = 
		{
			.Entity			= entity,
			.Position		= pECS->AddComponent<PositionComponent>(entity, { true, translation }),
			.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, glm::vec3(1.0f) }),
			.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
			.Mesh			= pECS->AddComponent<MeshComponent>(entity, meshComponent),
			.CollisionGroup = FCollisionGroup::COLLISION_GROUP_STATIC,
			.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
		};

		pPhysicsSystem->CreateCollisionTriangleMesh(collisionCreateInfo);
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

	bool LevelObjectCreator::CreateSpecialObjectOfType(ESpecialObjectType specialObjectType, const void* pData, TArray<Entity>& createdEntities)
	{
		auto createFuncIt = s_SpecialObjectByTypeCreateFunctions.find(specialObjectType);

		if (createFuncIt != s_SpecialObjectByTypeCreateFunctions.end())
		{
			return createFuncIt->second(pData, createdEntities);
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

	bool LevelObjectCreator::CreatePlayer(const void* pData, TArray<Entity>& createdEntities)
	{
		if (pData == nullptr) return false;

		using namespace LambdaEngine;

		const CreatePlayerDesc* pPlayerDesc = reinterpret_cast<const CreatePlayerDesc*>(pData);

		ECSCore* pECS = ECSCore::GetInstance();
		Entity playerEntity = pECS->CreateEntity();
		createdEntities.PushBack(playerEntity);

		glm::quat lookDirQuat = glm::quatLookAt(pPlayerDesc->Forward, g_DefaultUp);

		pECS->AddComponent<PlayerComponent>(playerEntity,	PlayerComponent{ .IsLocal = pPlayerDesc->IsLocal });
		pECS->AddComponent<PositionComponent>(playerEntity, PositionComponent{ .Position = pPlayerDesc->Position });
		pECS->AddComponent<RotationComponent>(playerEntity, RotationComponent{ .Quaternion = lookDirQuat });
		pECS->AddComponent<ScaleComponent>(playerEntity,	ScaleComponent{ .Scale = pPlayerDesc->Scale });
		pECS->AddComponent<VelocityComponent>(playerEntity, VelocityComponent());
	
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

		if (m_ClientSide)
		{
			//Todo: Set DrawArgs Mask here to avoid rendering local mesh
			pECS->AddComponent<MeshComponent>(playerEntity, pPlayerDesc->MeshComponent);
			pECS->AddComponent<AnimationComponent>(playerEntity, pPlayerDesc->AnimationComponent);

			if (pPlayerDesc->IsLocal)
			{
				if (pPlayerDesc->pCameraDesc == nullptr)
				{
					pECS->RemoveEntity(playerEntity);
					return false;
				}

				//Create Camera Entity
				Entity cameraEntity = pECS->CreateEntity();
				createdEntities.PushBack(cameraEntity);

				//Todo: Better implementation for this somehow maybe?
				const Mesh* pMesh = ResourceManager::GetMesh(pPlayerDesc->MeshComponent.MeshGUID);

				OffsetComponent offsetComponent = { .Offset = pPlayerDesc->Scale * glm::vec3(0.0f, 2.0f * pMesh->BoundingBox.HalfExtent.y, 0.0f) };

				pECS->AddComponent<OffsetComponent>(cameraEntity, offsetComponent);
				pECS->AddComponent<PositionComponent>(cameraEntity, PositionComponent{ .Position = pPlayerDesc->Position + offsetComponent.Offset });
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
		}

		D_LOG_INFO("Created Player");
		return true;
	}
}