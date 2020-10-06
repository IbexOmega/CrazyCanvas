#include "World/LevelObjectCreator.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"

#include "ECS/ECSCore.h"
#include "Physics/PhysicsSystem.h"

bool LevelObjectCreator::Init(bool clientSide)
{
	using namespace LambdaEngine;

	m_ClientSide = clientSide;

	//Spawnpoint
	{
		SpecialObjectDesc specialObjectDesc =
		{
			.Prefix			= "SPAWN_",
			.IncludeMesh	= true
		};

		s_SpecialObjectDescriptions.PushBack(specialObjectDesc);
		s_CreateFunctions[specialObjectDesc.Prefix] = &LevelObjectCreator::CreateSpawnpoint;
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
			.ColorIntensity = glm::vec4(directionalLight.Color, 1.0f)
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
			.ColorIntensity = glm::vec4(pointLight.Color, 1.0f)
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

bool LevelObjectCreator::CreateSpecialObject(const LambdaEngine::SpecialObject& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	auto createFuncIt = s_CreateFunctions.find(specialObject.Prefix);

	if (createFuncIt != s_CreateFunctions.end())
	{
		createFuncIt->second(specialObject, createdEntities, translation);
		return true;
	}
	else
	{
		LOG_ERROR("[LevelObjectCreator]: Failed to create entity %s with prefix %s", specialObject.Name, specialObject.Prefix);
		return false;
	}
}

void LevelObjectCreator::CreateSpawnpoint(const LambdaEngine::SpecialObject& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation)
{
	UNREFERENCED_VARIABLE(specialObject);
	UNREFERENCED_VARIABLE(createdEntities);
	UNREFERENCED_VARIABLE(translation);

	LOG_WARNING("[LevelObjectCreator]: Spawnpoint not implemented!");
}
