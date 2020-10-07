#pragma once

#include "LambdaEngine.h"

#include "Containers/TArray.h"
#include "Containers/String.h"

#include "Resources/ResourceLoader.h"

#include "ECS/Entity.h"

enum class ESpecialObjectType : uint8
{
	SPECIAL_OBJECT_TYPE_NONE		= 0,
	SPECIAL_OBJECT_TYPE_SPAWN_POINT	= 1,
	SPECIAL_OBJECT_TYPE_FLAG		= 2,
};

class LevelObjectCreator
{
	typedef ESpecialObjectType(*EntityCreateFunc)(const LambdaEngine::SpecialObject&, LambdaEngine::TArray<LambdaEngine::Entity>&, const glm::vec3&);

public:
	DECL_STATIC_CLASS(LevelObjectCreator);

	static bool Init(bool clientSide);

	static LambdaEngine::Entity CreateDirectionalLight(const LambdaEngine::LoadedDirectionalLight& directionalLight, const glm::vec3& translation);
	static LambdaEngine::Entity CreatePointLight(const LambdaEngine::LoadedPointLight& pointLight, const glm::vec3& translation);
	static LambdaEngine::Entity CreateStaticGeometry(const LambdaEngine::MeshComponent& meshComponent, const glm::vec3& translation);
	static ESpecialObjectType CreateSpecialObject(const LambdaEngine::SpecialObject& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation);

	FORCEINLINE static const LambdaEngine::TArray<LambdaEngine::SpecialObjectDesc>& GetSpecialObjectDescriptions() { return s_SpecialObjectDescriptions; }

private:
	static ESpecialObjectType CreateSpawnpoint(const LambdaEngine::SpecialObject& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation);
	static ESpecialObjectType CreateFlag(const LambdaEngine::SpecialObject& specialObject, LambdaEngine::TArray<LambdaEngine::Entity>& createdEntities, const glm::vec3& translation);

private:
	inline static bool m_ClientSide = false;
	inline static LambdaEngine::TArray<LambdaEngine::SpecialObjectDesc> s_SpecialObjectDescriptions;
	inline static LambdaEngine::THashTable<LambdaEngine::String, EntityCreateFunc> s_CreateFunctions;
};