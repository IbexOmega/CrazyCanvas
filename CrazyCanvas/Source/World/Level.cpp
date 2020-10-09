#include "World/Level.h"

#include "ECS/ECSCore.h"

Level::Level()
{
}

Level::~Level()
{
	using namespace LambdaEngine;
	ECSCore* pECS = ECSCore::GetInstance();

	for (LambdaEngine::Entity entity : m_Entities)
	{
		pECS->RemoveEntity(entity);
	}

	m_Entities.Clear();
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

		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = LevelObjectCreator::CreateStaticGeometry(meshComponent, translation);
			if (entity != UINT32_MAX) m_Entities.PushBack(entity);
		}

		if (!directionalLights.IsEmpty())
		{
			Entity entity = LevelObjectCreator::CreateDirectionalLight(directionalLights[0], translation);
			if (entity != UINT32_MAX) m_Entities.PushBack(entity);
		}

		for (const LoadedPointLight& loadedPointLight : pointLights)
		{
			Entity entity = LevelObjectCreator::CreatePointLight(loadedPointLight, translation);
			if (entity != UINT32_MAX) m_Entities.PushBack(entity);
		}

		TArray<Entity> newlyCreatedEntities;

		for (const SpecialObjectOnLoad& specialObject : specialObjects)
		{
			ESpecialObjectType specialObjectType = LevelObjectCreator::CreateSpecialObjectFromPrefix(specialObject, newlyCreatedEntities, translation);

			if (specialObjectType != ESpecialObjectType::SPECIAL_OBJECT_TYPE_NONE)
			{
				for (Entity entity : newlyCreatedEntities)
				{
					if (entity != UINT32_MAX)
					{
						m_EntityTypeMap[specialObjectType].PushBack(m_Entities.GetSize());
						m_Entities.PushBack(entity);
					}
				}
			}

			newlyCreatedEntities.Clear();
		}
	}

	return true;
}

bool Level::CreateObject(LambdaEngine::ESpecialObjectType specialObjectType, void* pData)
{
	using namespace LambdaEngine;

	if (specialObjectType != ESpecialObjectType::SPECIAL_OBJECT_TYPE_NONE)
	{
		TArray<Entity> newlyCreatedEntities;
		if (LevelObjectCreator::CreateSpecialObjectOfType(specialObjectType, pData, newlyCreatedEntities))
		{
			for (Entity entity : newlyCreatedEntities)
			{
				if (entity != UINT32_MAX)
				{
					m_EntityTypeMap[specialObjectType].PushBack(m_Entities.GetSize());
					m_Entities.PushBack(entity);
				}
			}

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
}

uint32 Level::GetEntityCount(LambdaEngine::ESpecialObjectType specialObjectType) const
{
	uint32 count = 0;

	auto entityIndicesIt = m_EntityTypeMap.find(specialObjectType);
	if (entityIndicesIt != m_EntityTypeMap.end())
	{
		count = entityIndicesIt->second.GetSize();
	}

	return count;
}
