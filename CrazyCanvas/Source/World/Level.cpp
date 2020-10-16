#include "World/Level.h"

#include "ECS/ECSCore.h"

Level::Level()
{
}

Level::~Level()
{
	using namespace LambdaEngine;
	ECSCore* pECS = ECSCore::GetInstance();

	for (LevelEntitiesOfType& levelEntities : m_Entities)
	{
		for (Entity entity : levelEntities.Entities)
		{
			pECS->RemoveEntity(entity);
		}

		for (TArray<Entity>& childEntities : levelEntities.ChildEntities)
		{
			for (Entity entity : childEntities)
			{
				pECS->RemoveEntity(entity);
			}
		}
	}

	m_Entities.Clear();
	m_EntityTypeMap.clear();
}

bool Level::Init(const LevelCreateDesc* pDesc)
{
	VALIDATE(pDesc != nullptr);

	m_Name = pDesc->Name;

	using namespace LambdaEngine;
	
	for (LevelModule* pModule : pDesc->LevelModules)
	{
		const glm::vec3& translation = pModule->GetTranslation();

		const TArray<MeshComponent>& meshComponents				= pModule->GetMeshComponents();
		const TArray<LoadedDirectionalLight>& directionalLights = pModule->GetDirectionalLights();
		const TArray<LoadedPointLight>& pointLights				= pModule->GetPointLights();
		const TArray<SpecialObjectOnLoad>& specialObjects		= pModule->GetSpecialObjects();

		LevelEntitiesOfType staticGeometryEntities;
		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = LevelObjectCreator::CreateStaticGeometry(meshComponent, translation);
			if (entity != UINT32_MAX) staticGeometryEntities.Entities.PushBack(entity);
		}
		m_EntityTypeMap[ESpecialObjectType::SPECIAL_OBJECT_TYPE_STATIC_GEOMTRY] = m_Entities.GetSize();
		m_Entities.PushBack(staticGeometryEntities);

		LevelEntitiesOfType dirLightEntities;
		if (!directionalLights.IsEmpty())
		{
			Entity entity = LevelObjectCreator::CreateDirectionalLight(directionalLights[0], translation);
			if (entity != UINT32_MAX) dirLightEntities.Entities.PushBack(entity);
		}
		m_EntityTypeMap[ESpecialObjectType::SPECIAL_OBJECT_TYPE_DIR_LIGHT] = m_Entities.GetSize();
		m_Entities.PushBack(dirLightEntities);

		LevelEntitiesOfType pointLightEntities;
		for (const LoadedPointLight& loadedPointLight : pointLights)
		{
			Entity entity = LevelObjectCreator::CreatePointLight(loadedPointLight, translation);
			if (entity != UINT32_MAX) pointLightEntities.Entities.PushBack(entity);
		}
		m_EntityTypeMap[ESpecialObjectType::SPECIAL_OBJECT_TYPE_POINT_LIGHT] = m_Entities.GetSize();
		m_Entities.PushBack(pointLightEntities);

		for (const SpecialObjectOnLoad& specialObject : specialObjects)
		{
			LevelEntitiesOfType levelEntities;
			ESpecialObjectType specialObjectType = LevelObjectCreator::CreateSpecialObjectFromPrefix(specialObject, levelEntities.Entities, translation);

			if (specialObjectType != ESpecialObjectType::SPECIAL_OBJECT_TYPE_NONE)
			{
				
				m_EntityTypeMap[specialObjectType] = m_Entities.GetSize();
				m_Entities.PushBack(levelEntities);
			}
		}
	}

	return true;
}

bool Level::CreateObject(ESpecialObjectType specialObjectType, void* pData)
{
	using namespace LambdaEngine;

	if (specialObjectType != ESpecialObjectType::SPECIAL_OBJECT_TYPE_NONE)
	{
		LevelEntitiesOfType* pLevelEntities = nullptr;

		auto specialObjectTypeIt = m_EntityTypeMap.find(specialObjectType);
		if (specialObjectTypeIt != m_EntityTypeMap.end())
		{
			pLevelEntities = &m_Entities[specialObjectTypeIt->second];
		}
		else
		{
			m_EntityTypeMap[specialObjectType] = m_Entities.GetSize();
			pLevelEntities = &m_Entities.PushBack({});
		}

		if (LevelObjectCreator::CreateSpecialObjectOfType(specialObjectType, pData, pLevelEntities->Entities, pLevelEntities->ChildEntities, pLevelEntities->SaltUIDs))
		{
			return true;
		}
	}

	return false;
}

void Level::SpawnPlayer(
	const LambdaEngine::MeshComponent& meshComponent, 
	const LambdaEngine::AnimationComponent& animationComponent, 
	const LambdaEngine::CameraDesc* pCameraDesc)
{
	UNREFERENCED_VARIABLE(meshComponent);
	UNREFERENCED_VARIABLE(animationComponent);
	UNREFERENCED_VARIABLE(pCameraDesc);
}

LambdaEngine::Entity* Level::GetEntities(ESpecialObjectType specialObjectType, uint32& countOut)
{
	auto specialObjectTypeIt = m_EntityTypeMap.find(specialObjectType);
	if (specialObjectTypeIt != m_EntityTypeMap.end())
	{
		LevelEntitiesOfType& levelEntities = m_Entities[specialObjectTypeIt->second];
		countOut = levelEntities.Entities.GetSize();
		return levelEntities.Entities.GetData();
	}

	return nullptr;
}

LambdaEngine::Entity Level::GetEntityPlayer(uint64 saltUID)
{
	using namespace LambdaEngine;

	auto specialObjectTypeIt = m_EntityTypeMap.find(ESpecialObjectType::SPECIAL_OBJECT_TYPE_PLAYER);
	if (specialObjectTypeIt != m_EntityTypeMap.end())
	{
		LevelEntitiesOfType& levelEntities = m_Entities[specialObjectTypeIt->second];
		for (uint32 i = 0; i < levelEntities.SaltUIDs.GetSize(); i++)
		{
			if (levelEntities.SaltUIDs[i] == saltUID)
			{
				return levelEntities.Entities[i];
			}
		}
	}

	return UINT32_MAX;
}
