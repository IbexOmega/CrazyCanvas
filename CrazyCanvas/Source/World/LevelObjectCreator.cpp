#include "World/LevelObjectCreator.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"

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
