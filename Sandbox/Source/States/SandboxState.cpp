#include "States/SandboxState.h"
#include "Log/Log.h"

#include "Resources/ResourceManager.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

using namespace LambdaEngine;

SandboxState::SandboxState()
{

}

SandboxState::SandboxState(LambdaEngine::State* pOther) : LambdaEngine::State(pOther)
{
}

SandboxState::~SandboxState()
{
	// Remove System
}

void SandboxState::Init()
{
	// Create Systems

	//Scene
	{
		TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("Map/scene.obj", meshComponents);

		glm::vec3 position(0.0f, 0.0f, 0.0f);
		glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
		glm::vec3 scale(1.0f);

		for (uint32 i = 0; i < meshComponents.GetSize(); i++)
		{
			//m_pScene->AddGameObject(entityID, meshComponents[i], transform, true, false);

			Entity entity = ECSCore::GetInstance()->CreateEntity();
			ECSCore::GetInstance()->AddComponent<PositionComponent>(entity, { position, true });
			ECSCore::GetInstance()->AddComponent<ScaleComponent>(entity, { scale, true });
			ECSCore::GetInstance()->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
			ECSCore::GetInstance()->AddComponent<MeshComponent>(entity, meshComponents[i]);
			ECSCore::GetInstance()->AddComponent<StaticComponent>(entity, StaticComponent());
		}
	}

	//Sphere Grid
	{
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

		uint32 gridRadius = 5;

		for (uint32 y = 0; y < gridRadius; y++)
		{
			float32 roughness = y / float32(gridRadius - 1);

			for (uint32 x = 0; x < gridRadius; x++)
			{
				float32 metallic = x / float32(gridRadius - 1);

				MaterialProperties materialProperties;
				materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				materialProperties.Roughness = roughness;
				materialProperties.Metallic = metallic;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID = sphereMeshGUID;
				sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
					"Default r: " + std::to_string(roughness) + " m: " + std::to_string(metallic),
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_NORMAL_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					materialProperties);

				glm::vec3 position(-float32(gridRadius) * 0.5f + x, 1.0f + y, 5.0f);
				glm::vec3 scale(1.0f);

				Entity entity = ECSCore::GetInstance()->CreateEntity();
				ECSCore::GetInstance()->AddComponent<PositionComponent>(entity, { position, true });
				ECSCore::GetInstance()->AddComponent<ScaleComponent>(entity, { scale, true });
				ECSCore::GetInstance()->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
				ECSCore::GetInstance()->AddComponent<MeshComponent>(entity, sphereMeshComp);
				ECSCore::GetInstance()->AddComponent<StaticComponent>(entity, StaticComponent());
			}
		}
	}

	// Load Scene SceneManager::Get("SceneName").Load()

	// Use HelperClass to create additional entities

	// EntityIndex index = HelperClass::CreatePlayer(
}

void SandboxState::Resume()
{
	// Unpause System

	// Reload Page
}

void SandboxState::Pause()
{
	// Pause System

	// Unload Page
}

void SandboxState::Tick(float dt)
{
	// Update State specfic objects
}
