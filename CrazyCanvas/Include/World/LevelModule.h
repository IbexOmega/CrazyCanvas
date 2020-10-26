#pragma once

#include "LambdaEngine.h"
#include "Containers/TArray.h"
#include "Containers/String.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Resources/ResourceLoader.h"

#include "World.h"

class LevelModule
{
public:
	LevelModule();
	~LevelModule();

	bool Init(const LambdaEngine::String& filename, const glm::vec3& translation);
	
	void SetTranslation(const glm::vec3& translation);

	FORCEINLINE const LambdaEngine::String& GetName() const { return m_Name; }
	FORCEINLINE const glm::vec3& GetTranslation() const { return m_Translation; }
	FORCEINLINE const LambdaEngine::TArray<LambdaEngine::MeshComponent>&			GetMeshComponents()		const { return m_MeshComponents; }
	FORCEINLINE const LambdaEngine::TArray<LambdaEngine::LoadedDirectionalLight>&	GetDirectionalLights()	const { return m_DirectionalLights; }
	FORCEINLINE const LambdaEngine::TArray<LambdaEngine::LoadedPointLight>&			GetPointLights()		const { return m_PointLights; }
	FORCEINLINE const LambdaEngine::TArray<LambdaEngine::LevelObjectOnLoad>&		GetLevelObjects()		const { return m_LevelObjects; }

private:
	LambdaEngine::String										m_Name;
	glm::vec3													m_Translation;
	LambdaEngine::TArray<LambdaEngine::MeshComponent>			m_MeshComponents;
	LambdaEngine::TArray<LambdaEngine::LoadedDirectionalLight>	m_DirectionalLights;
	LambdaEngine::TArray<LambdaEngine::LoadedPointLight>		m_PointLights;
	LambdaEngine::TArray<LambdaEngine::LevelObjectOnLoad>		m_LevelObjects;
};