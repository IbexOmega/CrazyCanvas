#include "States/SandboxState.h"
#include "Log/Log.h"

#include "Resources/ResourceManager.h"

#include "Engine/EngineConfig.h"

#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Systems/TrackSystem.h"

#include "Math/Random.h"

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
	TrackSystem::GetInstance().Init();

	// Create Camera
	{
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		CameraDesc cameraDesc = {};
		cameraDesc.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV");
		cameraDesc.Width = window->GetWidth();
		cameraDesc.Height = window->GetHeight();
		cameraDesc.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane");
		cameraDesc.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane");
		Entity e = CreateFreeCameraEntity(cameraDesc);
	}

	//Scene
	{
		TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("Testing/Testing.obj", meshComponents);

		glm::vec3 position(0.0f, 0.0f, 0.0f);
		glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
		glm::vec3 scale(1.0f);

		for (uint32 i = 0; i < meshComponents.GetSize(); i++)
		{
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

		// Directional Light
	/*	{
			m_DirLight = ECSCore::GetInstance()->CreateEntity();
			ECSCore::GetInstance()->AddComponent<RotationComponent>(m_DirLight, { glm::quatLookAt({1.0f, -1.0f, 0.0f}, g_DefaultUp), true });
			ECSCore::GetInstance()->AddComponent<DirectionalLightComponent>(m_DirLight, DirectionalLightComponent{ .ColorIntensity = {1.0f, 1.0f, 1.0f, 5.0f} });
		}*/
		
		// Add PointLights
		/*{
			constexpr uint32 POINT_LIGHT_COUNT = 3;
			const PointLightComponent pointLights[POINT_LIGHT_COUNT] =
			{
				{.ColorIntensity = {1.0f, 0.0f, 0.0f, 25.0f}},
				{.ColorIntensity = {0.0f, 1.0f, 0.0f, 25.0f}},
				{.ColorIntensity = {0.0f, 0.0f, 1.0f, 25.0f}},
			};

			const float PI = glm::pi<float>();
			const float RADIUS = 3.0f;
			for (uint32 i = 0; i < 3; i++)
			{
				TArray<glm::vec3> lightPath;
				lightPath.Reserve(36);
				float positive = std::pow(-1.0, i);

				glm::vec3 startPosition(0.0f, 0.0f, 5.0f);
				for (uint32 y = 0; y < 6; y++)
				{
					float angle = 0.f;
					for (uint32 x = 0; x < 6; x++)
					{
						glm::vec3 position = startPosition;
						angle += positive * (2.0f * PI / 6.0f);
						position.x += std::cos(angle) * RADIUS;
						position.z += std::sin(angle) * RADIUS;
						position.y += 1.0f + y * 0.5f + i * 2.0f;
						lightPath.PushBack(position);
					}
				}

				MaterialProperties materialProperties;
				glm::vec3 color = pointLights[i].ColorIntensity;
				materialProperties.Albedo = glm::vec4(color, 1.0f);
				materialProperties.Roughness = 0.1f;
				materialProperties.Metallic = 0.1f;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID = sphereMeshGUID;
				sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
					"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_NORMAL_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					materialProperties);

				m_PointLights[i] = ECSCore::GetInstance()->CreateEntity();
				ECSCore::GetInstance()->AddComponent<PositionComponent>(m_PointLights[i], { {0.0f, 0.0f, 0.0f}, true });
				ECSCore::GetInstance()->AddComponent<ScaleComponent>(m_PointLights[i], { glm::vec3(0.4f), true });
				ECSCore::GetInstance()->AddComponent<RotationComponent>(m_PointLights[i], { glm::identity<glm::quat>(), true });
				ECSCore::GetInstance()->AddComponent<PointLightComponent>(m_PointLights[i], pointLights[i]);
				ECSCore::GetInstance()->AddComponent<MeshComponent>(m_PointLights[i], sphereMeshComp);
				ECSCore::GetInstance()->AddComponent<DynamicComponent>(m_PointLights[i], DynamicComponent());
				ECSCore::GetInstance()->AddComponent<TrackComponent>(m_PointLights[i], TrackComponent{ .Track = lightPath });
			}
		}*/

		{
			constexpr uint32 POINT_LIGHT_COUNT = 30;

			const float PI = glm::pi<float>();
			const float RADIUS = 3.0f;
			for (uint32 i = 0; i < POINT_LIGHT_COUNT; i++)
			{
				TArray<glm::vec3> lightPath;
				lightPath.Reserve(36);
				float positive = std::pow(-1.0, i);

				PointLightComponent ptComp =
				{
					.ColorIntensity = glm::vec4(Random::Float32(0.0f, 1.0f), Random::Float32(0.0f, 1.0f), Random::Float32(0.0f, 1.0f), Random::Float32(1.0f, 10.0f))
				};

				glm::vec3 startPosition(0.0f, 0.0f, 5.0f + Random::Float32(-1.0f, 1.0f));
				for (uint32 y = 0; y < 6; y++)
				{
					float angle = 0.f;
					for (uint32 x = 0; x < 6; x++)
					{
						glm::vec3 position = startPosition;
						angle += positive * (2.0f * PI / 6.0f);
						position.x += std::cos(angle) * RADIUS;
						position.z += std::sin(angle) * RADIUS;
						position.y += 1.0f + y * Random::Float32(0.5f, 1.0f) + i * 0.1f;
						lightPath.PushBack(position);
					}
				}

				MaterialProperties materialProperties;
				glm::vec3 color = ptComp.ColorIntensity;
				materialProperties.Albedo = glm::vec4(color, 1.0f);
				materialProperties.Roughness = 0.1f;
				materialProperties.Metallic = 0.1f;

				MeshComponent sphereMeshComp = {};
				sphereMeshComp.MeshGUID = sphereMeshGUID;
				sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
					"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_NORMAL_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					GUID_TEXTURE_DEFAULT_COLOR_MAP,
					materialProperties);

				m_PointLights[i] = ECSCore::GetInstance()->CreateEntity();
				ECSCore::GetInstance()->AddComponent<PositionComponent>(m_PointLights[i], { {0.0f, 0.0f, 0.0f}, true });
				ECSCore::GetInstance()->AddComponent<ScaleComponent>(m_PointLights[i], { glm::vec3(0.4f), true });
				ECSCore::GetInstance()->AddComponent<RotationComponent>(m_PointLights[i], { glm::identity<glm::quat>(), true });
				ECSCore::GetInstance()->AddComponent<PointLightComponent>(m_PointLights[i], ptComp);
				ECSCore::GetInstance()->AddComponent<MeshComponent>(m_PointLights[i], sphereMeshComp);
				ECSCore::GetInstance()->AddComponent<DynamicComponent>(m_PointLights[i], DynamicComponent());
				ECSCore::GetInstance()->AddComponent<TrackComponent>(m_PointLights[i], TrackComponent{ .Track = lightPath });
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
	static float timer = 0;
	static int removedIndex = 0;

	timer += dt;

	if (timer > 0.5f && removedIndex < 30)
	{
		ECSCore* ecsCore = ECSCore::GetInstance();
		ecsCore->RemoveEntity(m_PointLights[removedIndex++]);

		timer = 0.0f;
	}
}
