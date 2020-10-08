#include "Game/World/LevelObjectCreator.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

#include "ECS/ECSCore.h"
#include "Physics/PhysicsSystem.h"

#include "Math/Math.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	bool LevelObjectCreator::Init(bool clientSide)
	{
		using namespace LambdaEngine;

		m_ClientSide = clientSide;

		//Spawnpoint
		{
			SpecialObjectDesc specialObjectDesc =
			{
				.Prefix			= "SO_SPAWN_"
			};

			s_SpecialObjectDescriptions.PushBack(specialObjectDesc);
			s_CreateFunctions[specialObjectDesc.Prefix] = &LevelObjectCreator::CreateSpawnpoint;
		}

		//Spawnpoint
		{
			SpecialObjectDesc specialObjectDesc =
			{
				.Prefix			= "SO_FLAG_"
			};

			s_SpecialObjectDescriptions.PushBack(specialObjectDesc);
			s_CreateFunctions[specialObjectDesc.Prefix] = &LevelObjectCreator::CreateFlag;
		}

		return true;
	}

	LambdaEngine::Entity LevelObjectCreator::CreatePlayer(
		bool isLocal,
		const glm::vec3& position,
		const glm::vec3& forward,
		const LambdaEngine::MeshComponent& meshComponent,
		const LambdaEngine::AnimationComponent& animationComponent,
		const LambdaEngine::CameraDesc* pCameraDesc)
	{
		using namespace LambdaEngine;

		ECSCore* pECS = ECSCore::GetInstance();
		Entity playerEntity = pECS->CreateEntity();

		glm::quat lookDirQuat = glm::quatLookAt(pCameraDesc->Direction, g_DefaultUp);

		pECS->AddComponent<PlayerComponent>(playerEntity,	PlayerComponent{ .IsLocal = isLocal });
		pECS->AddComponent<PositionComponent>(playerEntity, PositionComponent{ .Position = position });
		pECS->AddComponent<RotationComponent>(playerEntity, RotationComponent{ .Quaternion = lookDirQuat });
		pECS->AddComponent<ScaleComponent>(playerEntity,	ScaleComponent{ .Scale = {1.0f, 1.0f, 1.0f} });
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
			pECS->AddComponent<MeshComponent>(playerEntity, meshComponent);
			pECS->AddComponent<AnimationComponent>(playerEntity, animationComponent);

			if (isLocal)
			{
				VALIDATE(pCameraDesc != nullptr);

				//Create Camera Entity
				Entity cameraEntity = pECS->CreateEntity();

				//Todo: Better implementation for this somehow maybe?
				const Mesh* pMesh = ResourceManager::GetMesh(meshComponent.MeshGUID);

				OffsetComponent offsetComponent = { .Offset = glm::vec3(0.0f, 2.0f * pMesh->BoundingBox.HalfExtent.y, 0.0f) };

				pECS->AddComponent<OffsetComponent>(cameraEntity, offsetComponent);
				pECS->AddComponent<PositionComponent>(cameraEntity, PositionComponent{ .Position = position + offsetComponent.Offset });
				pECS->AddComponent<RotationComponent>(cameraEntity, RotationComponent{ .Quaternion = lookDirQuat });

				const ViewProjectionMatricesComponent viewProjComp = 
				{
					.Projection = glm::perspective(
						glm::radians(pCameraDesc->FOVDegrees), 
						pCameraDesc->Width / pCameraDesc->Height, 
						pCameraDesc->NearPlane, 
						pCameraDesc->FarPlane),

					.View = glm::lookAt(
						pCameraDesc->Position, 
						pCameraDesc->Position + pCameraDesc->Direction, 
						g_DefaultUp)
				};
				pECS->AddComponent<ViewProjectionMatricesComponent>(cameraEntity, viewProjComp);

				const CameraComponent cameraComp = 
				{
					.NearPlane	= pCameraDesc->NearPlane,
					.FarPlane	= pCameraDesc->FarPlane,
					.FOV		= pCameraDesc->FOVDegrees
				};
				pECS->AddComponent<CameraComponent>(cameraEntity, cameraComp);

				pECS->AddComponent<ParentComponent>(cameraEntity, ParentComponent{ .Parent = playerEntity, .Attached = true });
			}
		}

		return LambdaEngine::Entity();
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

	ESpecialObjectType LevelObjectCreator::CreateSpecialObject(const LambdaEngine::SpecialObject& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
	{
		auto createFuncIt = s_CreateFunctions.find(specialObject.Prefix);

		if (createFuncIt != s_CreateFunctions.end())
		{
			return createFuncIt->second(specialObject, createdEntities, translation);
		}
		else
		{
			LOG_ERROR("[LevelObjectCreator]: Failed to create entity %s with prefix %s", specialObject.Name, specialObject.Prefix);
			return ESpecialObjectType::SPECIAL_OBJECT_TYPE_NONE;
		}
	}

	ESpecialObjectType LevelObjectCreator::CreateSpawnpoint(const LambdaEngine::SpecialObject& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
	{
		UNREFERENCED_VARIABLE(specialObject);
		UNREFERENCED_VARIABLE(createdEntities);
		UNREFERENCED_VARIABLE(translation);

		LOG_WARNING("[LevelObjectCreator]: Spawnpoint not implemented!");
		return ESpecialObjectType::SPECIAL_OBJECT_TYPE_SPAWN_POINT;
	}

	ESpecialObjectType LevelObjectCreator::CreateFlag(const LambdaEngine::SpecialObject& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
	{
		UNREFERENCED_VARIABLE(specialObject);
		UNREFERENCED_VARIABLE(createdEntities);
		UNREFERENCED_VARIABLE(translation);

		LOG_WARNING("[LevelObjectCreator]: Create Flag not implemented!");
		return ESpecialObjectType::SPECIAL_OBJECT_TYPE_FLAG;
	}
}