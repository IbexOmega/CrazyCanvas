#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"
#include "Containers/String.h"

#include "ECS/Entity.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Resources/ResourceLoader.h"

class LevelModule
{
public:
	LevelModule();
	~LevelModule();

	bool Init(const LambdaEngine::String& filename);
	
	void RecreateEntities(const glm::vec3& translation);

private:
	LambdaEngine::String										m_Name;
	LambdaEngine::TArray<LambdaEngine::MeshComponent>			m_MeshComponents;
	LambdaEngine::TArray<LambdaEngine::LoadedDirectionalLight>	m_DirectionalLights;
	LambdaEngine::TArray<LambdaEngine::LoadedPointLight>		m_PointLights;
	LambdaEngine::TArray<LambdaEngine::SpecialObject>			m_SpecialObjects;

	LambdaEngine::TArray<LambdaEngine::Entity>					m_Entities;
};