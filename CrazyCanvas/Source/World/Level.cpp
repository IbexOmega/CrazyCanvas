#include "World/Level.h"

#include "ECS/ECSCore.h"

Level::Level()
{
}

Level::~Level()
{
	using namespace LambdaEngine;
	std::scoped_lock<SpinLock> lock(m_SpinLock);

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
	
	std::scoped_lock<SpinLock> lock(m_SpinLock);

	for (LevelModule* pModule : pDesc->LevelModules)
	{
		const glm::vec3& translation = pModule->GetTranslation();

		const TArray<MeshComponent>& meshComponents				= pModule->GetMeshComponents();
		const TArray<LoadedDirectionalLight>& directionalLights = pModule->GetDirectionalLights();
		const TArray<LoadedPointLight>& pointLights				= pModule->GetPointLights();
		const TArray<LevelObjectOnLoad>& levelObjects		= pModule->GetLevelObjects();

		LevelEntitiesOfType staticGeometryEntities;
		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = LevelObjectCreator::CreateStaticGeometry(meshComponent, translation);
			if (entity != UINT32_MAX) staticGeometryEntities.Entities.PushBack(entity);
		}
		m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_STATIC_GEOMTRY] = m_Entities.GetSize();
		m_Entities.PushBack(staticGeometryEntities);

		LevelEntitiesOfType dirLightEntities;
		if (!directionalLights.IsEmpty())
		{
			Entity entity = LevelObjectCreator::CreateDirectionalLight(directionalLights[0], translation);
			if (entity != UINT32_MAX) dirLightEntities.Entities.PushBack(entity);
		}
		m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_DIR_LIGHT] = m_Entities.GetSize();
		m_Entities.PushBack(dirLightEntities);

		LevelEntitiesOfType pointLightEntities;
		for (const LoadedPointLight& loadedPointLight : pointLights)
		{
			Entity entity = LevelObjectCreator::CreatePointLight(loadedPointLight, translation);
			if (entity != UINT32_MAX) pointLightEntities.Entities.PushBack(entity);
		}
		m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_POINT_LIGHT] = m_Entities.GetSize();
		m_Entities.PushBack(pointLightEntities);

		for (const LevelObjectOnLoad& levelObject : levelObjects)
		{
			LevelEntitiesOfType levelEntities;
			ELevelObjectType levelObjectType = LevelObjectCreator::CreateLevelObjectFromPrefix(levelObject, levelEntities.Entities, translation);

			if (levelObjectType != ELevelObjectType::LEVEL_OBJECT_TYPE_NONE)
			{
				m_EntityTypeMap[levelObjectType] = m_Entities.GetSize();
				m_Entities.PushBack(levelEntities);
			}
		}
	}

	return true;
}

bool Level::CreateObject(ELevelObjectType levelObjectType, const void* pData, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities)
{
	using namespace LambdaEngine;
	std::scoped_lock<SpinLock> lock(m_SpinLock);

	if (levelObjectType != ELevelObjectType::LEVEL_OBJECT_TYPE_NONE)
	{
		LevelEntitiesOfType* pLevelEntities = nullptr;

		auto levelObjectTypeIt = m_EntityTypeMap.find(levelObjectType);
		if (levelObjectTypeIt != m_EntityTypeMap.end())
		{
			pLevelEntities = &m_Entities[levelObjectTypeIt->second];
		}
		else
		{
			m_EntityTypeMap[levelObjectType] = m_Entities.GetSize();
			pLevelEntities = &m_Entities.PushBack({});
		}

		TArray<Entity> newEntities;
		if (LevelObjectCreator::CreateLevelObjectOfType(levelObjectType, pData, newEntities, pLevelEntities->ChildEntities, pLevelEntities->SaltUIDs))
		{
			pLevelEntities->Entities.Insert(pLevelEntities->Entities.End(), newEntities.Begin(), newEntities.End());
			createdEntities = newEntities;
			return true;
		}
	}

	return false;
}

LambdaEngine::Entity* Level::GetEntities(ELevelObjectType levelObjectType, uint32& countOut)
{
	using namespace LambdaEngine;
	std::scoped_lock<SpinLock> lock(m_SpinLock);

	auto levelObjectTypeIt = m_EntityTypeMap.find(levelObjectType);
	if (levelObjectTypeIt != m_EntityTypeMap.end())
	{
		LevelEntitiesOfType& levelEntities = m_Entities[levelObjectTypeIt->second];
		countOut = levelEntities.Entities.GetSize();
		return levelEntities.Entities.GetData();
	}

	return nullptr;
}

/*LambdaEngine::Entity Level::GetEntityPlayer(uint64 saltUID)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_SpinLock);

	auto levelObjectTypeIt = m_EntityTypeMap.find(ELevelObjectType::LEVEL_OBJECT_TYPE_PLAYER);
	if (levelObjectTypeIt != m_EntityTypeMap.end())
	{
		LevelEntitiesOfType& levelEntities = m_Entities[levelObjectTypeIt->second];
		for (uint32 i = 0; i < levelEntities.SaltUIDs.GetSize(); i++)
		{
			if (levelEntities.SaltUIDs[i] == saltUID)
			{
				return levelEntities.Entities[i];
			}
		}
	}

	return UINT32_MAX;
}*/
