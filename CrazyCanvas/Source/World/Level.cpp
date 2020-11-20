#include "World/Level.h"

#include "ECS/ECSCore.h"

Level::Level()
{
	SetComponentOwner<LevelRegisteredComponent>({ .Destructor = std::bind_front(&Level::LevelRegisteredDestructor, this) });
}

Level::~Level()
{
	using namespace LambdaEngine;
	std::scoped_lock<SpinLock> lock(m_SpinLock);

	ECSCore* pECS = ECSCore::GetInstance();

	m_EntityToLevelObjectTypeMap.clear();
	m_EntityTypeMap.clear();

	for (TArray<Entity>& levelEntitiesByType : m_LevelEntities)
	{
		for (Entity levelEntity : levelEntitiesByType)
		{
			pECS->RemoveEntity(levelEntity);
		}
	}

	m_LevelEntities.Clear();
}

bool Level::Init(const LevelCreateDesc* pDesc)
{
	VALIDATE(pDesc != nullptr);

	m_Name = pDesc->Name;

	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

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
					m_EntityToLevelObjectTypeMap[entity] = ELevelObjectType::LEVEL_OBJECT_TYPE_STATIC_GEOMETRY;
				}
			}

			if (!levelEntities.IsEmpty())
			{
				for (Entity entity : levelEntities)
				{
					pECS->AddComponent<LevelRegisteredComponent>(entity, LevelRegisteredComponent());
				}

				m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_STATIC_GEOMETRY] = m_LevelEntities.GetSize();
				m_LevelEntities.PushBack(levelEntities);
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
				for (Entity entity : levelEntities)
				{
					pECS->AddComponent<LevelRegisteredComponent>(entity, LevelRegisteredComponent());
				}

				m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_DIR_LIGHT] = m_LevelEntities.GetSize();
				m_LevelEntities.PushBack(levelEntities);
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
				for (Entity entity : levelEntities)
				{
					pECS->AddComponent<LevelRegisteredComponent>(entity, LevelRegisteredComponent());
				}

				m_EntityTypeMap[ELevelObjectType::LEVEL_OBJECT_TYPE_POINT_LIGHT] = m_LevelEntities.GetSize();
				m_LevelEntities.PushBack(levelEntities);
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
					TArray<Entity>* pLevelEntitiesOfType = nullptr;

					if (auto levelObjectTypeIt = m_EntityTypeMap.find(levelObjectType); levelObjectTypeIt != m_EntityTypeMap.end())
					{
						pLevelEntitiesOfType = &m_LevelEntities[levelObjectTypeIt->second];
					}
					else
					{
						m_EntityTypeMap[levelObjectType] = m_LevelEntities.GetSize();
						pLevelEntitiesOfType = &m_LevelEntities.PushBack({});
					}

					for (uint32 i = 0; i < entities.GetSize(); i++)
					{
						Entity entity = entities[i];
						pECS->AddComponent<LevelRegisteredComponent>(entity, LevelRegisteredComponent());

						pLevelEntitiesOfType->PushBack(entity);
						m_EntityToLevelObjectTypeMap[entity] = levelObjectType;
					}
				}
			}
		}
	}

	return true;
}

bool Level::CreateObject(ELevelObjectType levelObjectType, const void* pData, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities)
{
	using namespace LambdaEngine;
	std::scoped_lock<SpinLock> lock(m_SpinLock);

	ECSCore* pECS = ECSCore::GetInstance();

	if (levelObjectType != ELevelObjectType::LEVEL_OBJECT_TYPE_NONE)
	{
		TArray<Entity> newEntities;
		if (LevelObjectCreator::CreateLevelObjectOfType(levelObjectType, pData, newEntities))
		{
			TArray<Entity>* pLevelEntitiesOfType = nullptr;

			if (auto levelObjectTypeIt = m_EntityTypeMap.find(levelObjectType); levelObjectTypeIt != m_EntityTypeMap.end())
			{
				pLevelEntitiesOfType = &m_LevelEntities[levelObjectTypeIt->second];
			}
			else
			{
				m_EntityTypeMap[levelObjectType] = m_LevelEntities.GetSize();
				pLevelEntitiesOfType = &m_LevelEntities.PushBack({});
			}

			for (uint32 i = 0; i < newEntities.GetSize(); i++)
			{
				Entity entity = newEntities[i];
				pECS->AddComponent<LevelRegisteredComponent>(entity, LevelRegisteredComponent());

				pLevelEntitiesOfType->PushBack(entity);

				m_EntityToLevelObjectTypeMap[entity] = levelObjectType;
			}

			createdEntities = newEntities;
			return true;
		}
	}

	return false;
}

bool Level::DeleteObject(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();
	pECS->RemoveEntity(entity);

	return RegisterObjectDeletion(entity);
}

bool Level::RegisterObjectDeletion(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	std::scoped_lock<SpinLock> lock(m_SpinLock);

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
				return true;
			}
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

ELevelObjectType Level::GetLevelObjectType(LambdaEngine::Entity entity) const
{
	auto levelObjectTypeIt = m_EntityToLevelObjectTypeMap.find(entity);
	return levelObjectTypeIt != m_EntityToLevelObjectTypeMap.end() ? levelObjectTypeIt->second : ELevelObjectType::LEVEL_OBJECT_TYPE_NONE;
}

void Level::LevelRegisteredDestructor(LevelRegisteredComponent& levelRegisteredComponent, LambdaEngine::Entity entity)
{
	UNREFERENCED_VARIABLE(levelRegisteredComponent);

	LOG_ERROR("[LevelRegisteredDestructor]: Deleting Entity: %u", entity);
	RegisterObjectDeletion(entity);
}
