#include "States/SandboxState.h"

#include "Resources/ResourceManager.h"

#include "Engine/EngineConfig.h"

#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Input/API/Input.h"
#include "Math/Random.h"
#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Systems/TrackSystem.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/Core/API/GraphicsTypes.h"

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
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SandboxState::OnKeyPressed);
	ECSCore* pECS = ECSCore::GetInstance();

	// Create Camera
	{
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		CameraDesc cameraDesc = {};
		cameraDesc.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV");
		cameraDesc.Width = window->GetWidth();
		cameraDesc.Height = window->GetHeight();
		cameraDesc.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane");
		cameraDesc.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane");
		CreateFreeCameraEntity(cameraDesc);
	}

	// Load scene
	//{
	//	TArray<MeshComponent> meshComponents;
	//	ResourceManager::LoadSceneFromFile("sponza/sponza.obj", meshComponents);

	//	const glm::vec3 position(0.0f, 0.0f, 0.0f);
	//	const glm::vec3 scale(0.01f);

	//	for (const MeshComponent& meshComponent : meshComponents)
	//	{
	//		Entity entity = ECSCore::GetInstance()->CreateEntity();
	//		pECS->AddComponent<PositionComponent>(entity, { position, true });
	//		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
	//		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
	//		pECS->AddComponent<MeshComponent>(entity, meshComponent);
	//		m_Entities.PushBack(entity);
	//	}
	//}


	//Scene
	//{
	//	TArray<MeshComponent> meshComponents;
	//	ResourceManager::LoadSceneFromFile("Testing/Testing.obj", meshComponents);

	//	glm::vec3 position(0.0f, 0.0f, 0.0f);
	//	glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
	//	glm::vec3 scale(1.0f);

	//	for (const MeshComponent& meshComponent : meshComponents)
	//	{
	//		Entity entity = ECSCore::GetInstance()->CreateEntity();
	//		pECS->AddComponent<PositionComponent>(entity, { position, true });
	//		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
	//		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
	//		pECS->AddComponent<MeshComponent>(entity, meshComponent);
	//	}
	//}

	//Sphere Grid
	{
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
		
		MaterialProperties materialProperties;
		materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		materialProperties.Roughness = 0.5f;
		materialProperties.Metallic = 0.0f;

		MeshComponent sphereMeshComp = {};
		sphereMeshComp.MeshGUID = sphereMeshGUID;
		sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Default ",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { {2.0f, 1.0f, 0.0f}, true });
		pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(0.1f), true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { {-2.0f, 1.0f, 0.0f}, true });
		pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(0.2f), true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { {0.0f, 1.0f, -2.0f}, true });
		pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(0.3f), true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { {0.0f, 1.0f, 2.0f}, true });
		pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(0.4f), true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);


		/*uint32 gridRadius = 2;

		for (uint32 y = 0; y < 4 * 0.5; y++)
		{
			float32 roughness = y / float32(gridRadius - 1);

			for (uint32 x = 0; x < 5; x++)
			{
				float32 metallic = x / float32(gridRadius - 1);

				MaterialProperties materialProperties;
				materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				materialProperties.Roughness = 0.4f;
				materialProperties.Metallic = 0.0f;

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

				for (uint32 z = 0; z < 5; z++)
				{
					glm::vec3 position(-4.0 * 0.5f + x * 2.0f, 1.0f + y * 4.0f, -2.0f + z * 2.0f);
					glm::vec3 scale(1.0f);

					Entity entity = pECS->CreateEntity();
					pECS->AddComponent<PositionComponent>(entity, { position, true });
					pECS->AddComponent<ScaleComponent>(entity, { scale, true });
					pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
					pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);
				m_Entities.PushBack(entity);


					glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), position);
					transform *= glm::toMat4(glm::identity<glm::quat>());
					transform = glm::scale(transform, scale);
				}
			}*/
		}

		// Directional Light
		/*{
			uint32 gun = ResourceManager::LoadMeshFromFile("gun.obj");
			MaterialProperties materialProperties;
			materialProperties.Albedo = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
			materialProperties.Roughness = 0.01f;
			materialProperties.Metallic = 0.f;

			MeshComponent sphereMeshComp = {};
			sphereMeshComp.MeshGUID = gun;
			sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
				"DIR_LIGHT",
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_NORMAL_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				materialProperties);

			m_DirLight = ECSCore::GetInstance()->CreateEntity();
			glm::vec3 direction(5.0f, -10.0f, -5.0f);
			pECS->AddComponent<PositionComponent>(m_DirLight, { glm::vec3(-5.0f, 10.0f, 5.0f), true });
			pECS->AddComponent<RotationComponent>(m_DirLight, { glm::quatLookAt(glm::normalize(direction), g_DefaultUp), true });
			pECS->AddComponent<ScaleComponent>(m_DirLight, { glm::vec3(2.0f), true });
			pECS->AddComponent<MeshComponent>(m_DirLight, sphereMeshComp);
			pECS->AddComponent<DirectionalLightComponent>(m_DirLight, DirectionalLightComponent{ .ColorIntensity = {1.0f, 1.0f, 1.0f, 5.0f} });
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

			const glm::vec3 startPosition[3] =
			{
				{1.0f, 3.0f, 0.0f},
				{0.0f, 1.0f, 0.0f},
				{0.0f, 0.1f, 1.0f},
			};

			const float PI = glm::pi<float>();
			const float RADIUS = 3.0f;
			for (uint32 i = 0; i < 3; i++)
			{
				float positive = std::pow(-1.0, i);

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

				m_PointLights[i] = pECS->CreateEntity();
				m_Entities.PushBack(m_PointLights[i]);
				pECS->AddComponent<PositionComponent>(m_PointLights[i], { startPosition[i], true });
				pECS->AddComponent<ScaleComponent>(m_PointLights[i], { glm::vec3(0.4f), true });
				pECS->AddComponent<RotationComponent>(m_PointLights[i], { glm::identity<glm::quat>(), true });
				pECS->AddComponent<PointLightComponent>(m_PointLights[i], pointLights[i]);
				pECS->AddComponent<MeshComponent>(m_PointLights[i], sphereMeshComp);
			}
		}*/

		{
			uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
			constexpr uint32 POINT_LIGHT_COUNT = 1;

			for (uint32 i = 0; i < POINT_LIGHT_COUNT; i++)
			{
				PointLightComponent ptComp =
				{
					.ColorIntensity = glm::vec4(glm::vec3(1.0f, 0.f, 0.f), 25.0f)
				};

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

				m_PointLights[i] = pECS->CreateEntity();
				pECS->AddComponent<PositionComponent>(m_PointLights[i], { {0.0f, 1.5f, 0.0f}, true });
				pECS->AddComponent<ScaleComponent>(m_PointLights[i], { glm::vec3(0.4f), true });
				pECS->AddComponent<RotationComponent>(m_PointLights[i], { glm::identity<glm::quat>(), true });
				pECS->AddComponent<PointLightComponent>(m_PointLights[i], ptComp);
				pECS->AddComponent<MeshComponent>(m_PointLights[i], sphereMeshComp);
				//pECS->AddComponent<TrackComponent>(m_PointLights[i], TrackComponent{ .Track = lightPath });
			}
		}

		//Mirrors
		{
			MaterialProperties mirrorProperties = {};
			mirrorProperties.Roughness = 1.0f;

			MeshComponent meshComponent;
			meshComponent.MeshGUID = GUID_MESH_QUAD;
			meshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
				"Mirror Material",
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_NORMAL_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				mirrorProperties);


			Entity entity = ECSCore::GetInstance()->CreateEntity();

			pECS->AddComponent<PositionComponent>(entity, { glm::vec3(0.f), true });
			pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
			pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(40.0f), true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
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

void SandboxState::Tick(LambdaEngine::Timestamp delta)
{
	 //Update State specfic objects
	ECSCore* pECS = ECSCore::GetInstance();

	auto& ptComp = pECS->GetComponent<PointLightComponent>(m_PointLights[0]);
	ptComp.Dirty = true;
}

bool SandboxState::OnKeyPressed(const LambdaEngine::KeyPressedEvent& event)
{
	using namespace LambdaEngine;

	if (event.Key == EKey::KEY_6)
	{
		int32 entityIndex = Random::Int32(0, int32(m_Entities.GetSize() - 1));
		Entity entity = m_Entities[entityIndex];
		m_Entities.Erase(m_Entities.Begin() + entityIndex);
		ECSCore::GetInstance()->RemoveEntity(entity);
	}

	return true;
}
