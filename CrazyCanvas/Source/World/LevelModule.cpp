#include "World/LevelModule.h"
#include "World/LevelObjectCreator.h"

#include "Resources/ResourceManager.h"

LevelModule::LevelModule()
{
}

LevelModule::~LevelModule()
{
	using namespace LambdaEngine;

	for (const MeshComponent& meshComponent : m_MeshComponents)
	{
		ResourceManager::UnloadMesh(meshComponent.MeshGUID);
		ResourceManager::UnloadMaterial(meshComponent.MaterialGUID);
	}
}

bool LevelModule::Init(const LambdaEngine::String& filename, const glm::vec3& translation)
{
	using namespace LambdaEngine;

	m_Name			= filename;
	m_Translation	= translation;

	SceneLoadDesc loadDesc =
	{
		.Filename					= filename,
		.LevelObjectDescriptions	= LevelObjectCreator::GetLevelObjectOnLoadDescriptions()
	};

	return ResourceManager::LoadSceneFromFile(&loadDesc, m_MeshComponents, m_DirectionalLights, m_PointLights, m_LevelObjects, LEVEL_MODULES_DIRECTORY);
}

void LevelModule::SetTranslation(const glm::vec3& translation)
{
	m_Translation = translation;
}