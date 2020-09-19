#include "States/SandboxState.h"
#include "Log/Log.h"

#include "Resources/ResourceManager.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Systems/TrackSystem.h"

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
	//TrackSystem::GetInstance().Init();

	//Scene
	{
		TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("Testing/Testing.obj", meshComponents);

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
				materialProperties.Roughness = std::max(roughness, 0.1f);
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
		//Entity entity = ECSCore::GetInstance()->CreateEntity();
		//ECSCore::GetInstance()->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		//ECSCore::GetInstance()->AddComponent<DirectionalLightComponent>(entity, DirectionalLightComponent{ .ColorIntensity = {1.0f, 0.0f, 0.0f, 1.0f} });

		std::vector<glm::vec3> lightPath = {
			{-2.0f, 1.6f, 1.0f},
			{9.8f, 1.6f, 0.8f},
			{9.4f, 1.6f, -3.8f},
			{-9.8f, 1.6f, -3.9f},
			{-11.6f, 1.6f, -1.1f},
			{9.8f, 6.1f, -0.8f},
			{9.4f, 6.1f, 3.8f},
			{-9.8f, 6.1f, 3.9f}
		};


		Entity entity = ECSCore::GetInstance()->CreateEntity();
		//ECSCore::GetInstance()->AddComponent<TrackComponent>(entity, TrackComponent{ .Track = lightPath });
		ECSCore::GetInstance()->AddComponent<PositionComponent>(entity, { {0.0f, 0.2f, 7.0f}, true });
		ECSCore::GetInstance()->AddComponent<ScaleComponent>(entity, { glm::vec3(1.0f), true });
		ECSCore::GetInstance()->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		ECSCore::GetInstance()->AddComponent<PointLightComponent>(entity, PointLightComponent{ .ColorIntensity = {1.0f, 0.0f, 0.0f, 25.0f} });

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
