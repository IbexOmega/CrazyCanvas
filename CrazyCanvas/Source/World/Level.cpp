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

	for (TArray<Entity>& levelEntitiesByType : m_LevelEntities)
	{
		for (Entity levelEntity : levelEntitiesByType)
		{
			pECS->RemoveEntity(levelEntity);
		}
	}

	for (TArray<TArray<Entity>>& levelChildEntitiesByType : m_LevelChildEntities)
	{
		for (TArray<Entity>& levelChildEntities : levelChildEntitiesByType)
		{
			for (Entity childEntity : levelChildEntities)
			{
				pECS->RemoveEntity(childEntity);
			}
		}
	}

	m_EntityToLevelObjectTypeMap.clear();
	m_EntityTypeMap.clear();

	m_LevelEntities.Clear();
	m_LevelChildEntities.Clear();
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
		const TArray<LevelObjectOnLoad>& uniqueLevelObjects		= pModule->GetLevelObjects();

		//Load Geometry
		{
			TArray<Entity> levelEntities;
			for (const MeshComponent& meshComponent : meshComponents)
			{
				Entity entity = LevelObjectCreator::CreateStaticGeometry(meshComponent, translation);

				if (entity != UINT32_MAX)
				{
					levelEntities.PushBack(entity);
					m_EntityToLevelObjectTypeMap[entity] = ELevelObjectType::LEVEL_OBJECT_TYPE_STATIC_GEOMTRY;
				}
			}

			if (!levelEntities.IsEmpty())
			{
				m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_STATIC_GEOMTRY] = m_LevelEntities.GetSize();
				m_LevelEntities.PushBack(levelEntities);
				m_LevelChildEntities.PushBack({});
			}
		}

		//Load Dir Lights
		{
			TArray<Entity> levelEntities;
			if (!directionalLights.IsEmpty())
			{
				Entity entity = LevelObjectCreator::CreateDirectionalLight(directionalLights[0], translation);

				if (entity != UINT32_MAX)
				{
					levelEntities.PushBack(entity);
					m_EntityToLevelObjectTypeMap[entity] = ELevelObjectType::LEVEL_OBJECT_TYPE_DIR_LIGHT;
				}
			}

			if (!levelEntities.IsEmpty())
			{
				m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_DIR_LIGHT] = m_LevelEntities.GetSize();
				m_LevelEntities.PushBack(levelEntities);
				m_LevelChildEntities.PushBack({});
			}
		}

		//Load Dir Lights
		{
			TArray<Entity> levelEntities;
			for (const LoadedPointLight& loadedPointLight : pointLights)
			{
				Entity entity = LevelObjectCreator::CreatePointLight(loadedPointLight, translation);

				if (entity != UINT32_MAX)
				{
					levelEntities.PushBack(entity);
					m_EntityToLevelObjectTypeMap[entity] = ELevelObjectType::LEVEL_OBJECT_TYPE_POINT_LIGHT;
				}
			}

			if (!levelEntities.IsEmpty())
			{
				m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_POINT_LIGHT] = levelEntities.GetSize();
				m_LevelEntities.PushBack(levelEntities);
				m_LevelChildEntities.PushBack({});
			}
		}

		//Load Unique Level Objects
		{
			for (const LevelObjectOnLoad& unqiueLevelObject : uniqueLevelObjects)
			{
				TArray<Entity> entities;
				ELevelObjectType levelObjectType = LevelObjectCreator::CreateLevelObjectFromPrefix(unqiueLevelObject, entities, translation);

				if (levelObjectType != ELevelObjectType::LEVEL_OBJECT_TYPE_NONE)
				{
					if (auto levelObjectTypeIt = m_EntityTypeMap.find(levelObjectType); levelObjectTypeIt != m_EntityTypeMap.end())
					{
						TArray<Entity>& levelEntitiesOfType = m_LevelEntities[levelObjectTypeIt->second];
						
						for (uint32 i = 0; i < entities.GetSize(); i++)
						{
							Entity entity = entities[i];
							levelEntitiesOfType.PushBack(entity);
							m_EntityToLevelObjectTypeMap[entity] = levelObjectType;
						}
					}
					else
					{
						m_EntityTypeMap[levelObjectType] = m_LevelEntities.GetSize();
						TArray<Entity>& levelEntitiesOfType = m_LevelEntities.PushBack({});
						m_LevelChildEntities.PushBack({});

						for (uint32 i = 0; i < entities.GetSize(); i++)
						{
							Entity entity = entities[i];
							levelEntitiesOfType.PushBack(entity);
							m_EntityToLevelObjectTypeMap[entity] = levelObjectType;
						}
					}
				}
			}
		}
	}

	return true;
}

bool Level::DeleteObject(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	auto levelObjectTypeIt = m_EntityToLevelObjectTypeMap.find(entity);
	if (levelObjectTypeIt != m_EntityToLevelObjectTypeMap.end())
	{
		auto entityTypeMapIt = m_EntityTypeMap.find(levelObjectTypeIt->second);
		if (entityTypeMapIt != m_EntityTypeMap.end())
		{
			uint32 index = entityTypeMapIt->second;
			TArray<Entity>& entities = m_LevelEntities[index];
			int32 entityIndex = -1;
			for (uint32 i = 0; i < entities.GetSize(); i++)
			{
				if (entities[i] == entity)
				{
					entityIndex = (int32)i;
					break;
				}
			}

			if (entityIndex >= 0)
			{
				entities.Erase(entities.Begin() + entityIndex);
				TArray<TArray<Entity>>& entitiesWithChildren = m_LevelChildEntities[index];
				TArray<Entity>& childEntities = entitiesWithChildren[entityIndex];

				for (uint32 i = 0; i < childEntities.GetSize(); i++)
				{
					pECS->RemoveEntity(childEntities[i]);
				}

				entitiesWithChildren.Erase(entitiesWithChildren.begin() + entityIndex);

				pECS->RemoveEntity(entity);
				return true;
			}
		}
	}

	return false;
}

bool Level::CreateObject(ELevelObjectType levelObjectType, const void* pData, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities)
{
	using namespace LambdaEngine;
	std::scoped_lock<SpinLock> lock(m_SpinLock);

	if (levelObjectType != ELevelObjectType::LEVEL_OBJECT_TYPE_NONE)
	{
		TArray<uint64> newSaltUIDs;
		TArray<Entity> newEntities;
		TArray<TArray<Entity>> newChildEntities;
		if (LevelObjectCreator::CreateLevelObjectOfType(levelObjectType, pData, newEntities, newChildEntities))
		{
			if (auto levelObjectTypeIt = m_EntityTypeMap.find(levelObjectType); levelObjectTypeIt != m_EntityTypeMap.end())
			{
				TArray<Entity>& levelEntitiesOfType = m_LevelEntities[levelObjectTypeIt->second];
				TArray<TArray<Entity>>& levelChildEntitiesOfType = m_LevelChildEntities[levelObjectTypeIt->second];

				for (uint32 i = 0; i < newEntities.GetSize(); i++)
				{
					Entity entity = newEntities[i];
					levelEntitiesOfType.PushBack(entity);

					if (!newChildEntities.IsEmpty())
					{
						levelChildEntitiesOfType.PushBack(newChildEntities[i]);
					}
					else
					{
						levelChildEntitiesOfType.PushBack({});
					}

					m_EntityToLevelObjectTypeMap[entity] = levelObjectType;
				}
			}
			else
			{
				m_EntityTypeMap[levelObjectType] = m_LevelEntities.GetSize();
				TArray<Entity>& levelEntitiesOfType = m_LevelEntities.PushBack({});
				TArray<TArray<Entity>>& levelChildEntitiesOfType = m_LevelChildEntities.PushBack({});

				for (uint32 i = 0; i < newEntities.GetSize(); i++)
				{
					Entity entity = newEntities[i];
					levelEntitiesOfType.PushBack(entity);

					if (!newChildEntities.IsEmpty())
					{
						levelChildEntitiesOfType.PushBack(newChildEntities[i]);
					}
					else
					{
						levelChildEntitiesOfType.PushBack({});
					}

					m_EntityToLevelObjectTypeMap[entity] = levelObjectType;
				}
			}

			createdEntities = newEntities;
			return true;
		}
	}

	return false;
}

LambdaEngine::TArray<LambdaEngine::Entity> Level::GetEntities(ELevelObjectType levelObjectType)
{
	using namespace LambdaEngine;
	std::scoped_lock<SpinLock> lock(m_SpinLock);

	auto levelObjectTypeIt = m_EntityTypeMap.find(levelObjectType);
	if (levelObjectTypeIt != m_EntityTypeMap.end())
	{
		return m_LevelEntities[levelObjectTypeIt->second];
	}

	return {};
}

LambdaEngine::TArray<LambdaEngine::TArray<LambdaEngine::Entity>> Level::GetChildEntities(ELevelObjectType levelObjectType)
{
	using namespace LambdaEngine;
	std::scoped_lock<SpinLock> lock(m_SpinLock);

	auto levelObjectTypeIt = m_EntityTypeMap.find(levelObjectType);
	if (levelObjectTypeIt != m_EntityTypeMap.end())
	{
		return m_LevelChildEntities[levelObjectTypeIt->second];
	}

	return {};
}