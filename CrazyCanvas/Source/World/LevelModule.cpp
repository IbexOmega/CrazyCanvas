#include "World/LevelModule.h"

#include "Resources/ResourceManager.h"

#include "World/LevelObjectCreator.h"

#include "ECS/ECSCore.h"

LevelModule::LevelModule()
{
}

LevelModule::~LevelModule()
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	for (LambdaEngine::Entity entity : m_Entities)
	{
		pECS->RemoveEntity(entity);
	}

	for (const MeshComponent& meshComponent : m_MeshComponents)
	{
		ResourceManager::UnloadMesh(meshComponent.MeshGUID);
		ResourceManager::UnloadMaterial(meshComponent.MaterialGUID);
	}
}

bool LevelModule::Init(const LambdaEngine::String& filename)
{
	using namespace LambdaEngine;

	m_Name = filename;

	SceneLoadDesc loadDesc =
	{
		.Filename					= filename,
		.SpecialObjectDescriptions	= LevelObjectCreator::GetSpecialObjectDescriptions()
	};

	return ResourceManager::LoadSceneFromFile(&loadDesc, m_MeshComponents, m_DirectionalLights, m_PointLights, m_SpecialObjects, "../Assets/World/Levels/");
}

void LevelModule::RecreateEntities(const glm::vec3& translation)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	for (LambdaEngine::Entity entity : m_Entities)
	{
		pECS->RemoveEntity(entity);
	}

	m_Entities.Clear();

	for (const MeshComponent& meshComponent : m_MeshComponents)
	{
		Entity entity = LevelObjectCreator::CreateStaticGeometry(meshComponent, translation);
		if (entity != UINT32_MAX) m_Entities.PushBack(entity);
	}

	if (!m_DirectionalLights.IsEmpty())
	{
		Entity entity = LevelObjectCreator::CreateDirectionalLight(m_DirectionalLights[0], translation);		
		if (entity != UINT32_MAX) m_Entities.PushBack(entity);
	}

	for (const LoadedPointLight& loadedPointLight : m_PointLights)
	{
		Entity entity = LevelObjectCreator::CreatePointLight(loadedPointLight, translation);
		if (entity != UINT32_MAX) m_Entities.PushBack(entity);
	}

	TArray<Entity> newlyCreatedEntities;

	for (const SpecialObject& specialObject : m_SpecialObjects)
	{
		if (LevelObjectCreator::CreateSpecialObject(specialObject, newlyCreatedEntities, translation))
		{
			for (Entity entity : newlyCreatedEntities)
			{
				if (entity != UINT32_MAX) m_Entities.PushBack(entity);
			}
		}

		newlyCreatedEntities.Clear();
	}
}