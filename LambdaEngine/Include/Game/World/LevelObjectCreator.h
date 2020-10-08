#pragma once

#include "LambdaEngine.h"

#include "Containers/TArray.h"
#include "Containers/String.h"

#include "Resources/ResourceLoader.h"

#include "ECS/Entity.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"

namespace LambdaEngine
{
	class CameraDesc;

	enum class ESpecialObjectType : uint8
	{
		SPECIAL_OBJECT_TYPE_NONE		= 0,
		SPECIAL_OBJECT_TYPE_SPAWN_POINT	= 1,
		SPECIAL_OBJECT_TYPE_FLAG		= 2,
	};

	class LevelObjectCreator
	{
		typedef ESpecialObjectType(*EntityCreateFunc)(const SpecialObject&, TArray<Entity>&, const glm::vec3&);

		static constexpr const float PLAYER_CAPSULE_HEIGHT = 1.0f;
		static constexpr const float PLAYER_CAPSULE_RADIUS = 0.2f;

	public:
		DECL_STATIC_CLASS(LevelObjectCreator);

		static bool Init(bool clientSide);

		static Entity CreatePlayer(
			bool isLocal,
			const glm::vec3& position,
			const glm::vec3& forward,
			const MeshComponent& meshComponent,
			const AnimationComponent& animationComponent,
			const CameraDesc* pCameraDesc);
		static Entity CreateDirectionalLight(const LoadedDirectionalLight& directionalLight, const glm::vec3& translation);
		static Entity CreatePointLight(const LoadedPointLight& pointLight, const glm::vec3& translation);
		static Entity CreateStaticGeometry(const MeshComponent& meshComponent, const glm::vec3& translation);
		static ESpecialObjectType CreateSpecialObject(const SpecialObject& specialObject, TArray<Entity>& createdEntities, const glm::vec3& translation);

		FORCEINLINE static const TArray<SpecialObjectDesc>& GetSpecialObjectDescriptions() { return s_SpecialObjectDescriptions; }

	private:
		static ESpecialObjectType CreateSpawnpoint(const SpecialObject& specialObject, TArray<Entity>& createdEntities, const glm::vec3& translation);
		static ESpecialObjectType CreateFlag(const SpecialObject& specialObject, TArray<Entity>& createdEntities, const glm::vec3& translation);

	private:
		inline static bool m_ClientSide = false;
		inline static TArray<SpecialObjectDesc> s_SpecialObjectDescriptions;
		inline static THashTable<String, EntityCreateFunc> s_CreateFunctions;
	};
}