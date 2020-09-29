#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"
#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Input/API/Input.h"

void PlaySessionState::Init()
{
	using namespace LambdaEngine;

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= EngineConfig::GetFloatProperty("CameraFOV");
	cameraDesc.Width		= window->GetWidth();
	cameraDesc.Height		= window->GetHeight();
	cameraDesc.NearPlane	= EngineConfig::GetFloatProperty("CameraNearPlane");
	cameraDesc.FarPlane		= EngineConfig::GetFloatProperty("CameraFarPlane");
	cameraDesc.Position = glm::vec3(0.f, 3.f, 0.f);
	CreateFreeCameraEntity(cameraDesc);

	ECSCore* pECS = ECSCore::GetInstance();

	// Scene
	{
		TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("Prototype/PrototypeScene.dae", meshComponents);

		glm::vec3 position(0.0f, 0.0f, 0.0f);
		glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
		glm::vec3 scale(1.0f);

		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = ECSCore::GetInstance()->CreateEntity();
			pECS->AddComponent<PositionComponent>(entity, { position, true });
			pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
			pECS->AddComponent<ScaleComponent>(entity, { scale, true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
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

				glm::vec3 position(-float32(gridRadius) * 0.5f + x, 2.0f + y, 5.0f);
				glm::vec3 scale(1.0f);

				Entity entity = pECS->CreateEntity();
				pECS->AddComponent<PositionComponent>(entity, { position, true });
				pECS->AddComponent<ScaleComponent>(entity, { scale, true });
				pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
				pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);
			}
		}

		// Directional Light
		//{
		//	Entity dirLight = ECSCore::GetInstance()->CreateEntity();
		//	ECSCore::GetInstance()->AddComponent<PositionComponent>(dirLight, { { 0.0f, 0.0f, 0.0f} });
		//	ECSCore::GetInstance()->AddComponent<RotationComponent>(dirLight, { glm::quatLookAt({1.0f, -1.0f, 0.0f}, g_DefaultUp), true });
		//	ECSCore::GetInstance()->AddComponent<DirectionalLightComponent>(dirLight, DirectionalLightComponent{ .ColorIntensity = {1.0f, 1.0f, 1.0f, 5.0f} });
		//}

		// Add PointLights
		{
			constexpr uint32 POINT_LIGHT_COUNT = 3;
			const PointLightComponent pointLights[POINT_LIGHT_COUNT] =
			{
				{.ColorIntensity = {1.0f, 0.0f, 0.0f, 25.0f}, .FarPlane = 20.0f},
				{.ColorIntensity = {0.0f, 1.0f, 0.0f, 25.0f}, .FarPlane = 20.0f},
				{.ColorIntensity = {0.0f, 0.0f, 1.0f, 25.0f}, .FarPlane = 20.0f},
			};

			const glm::vec3 startPosition[3] =
			{
				{4.0f, 2.0f, -3.0f},
				{-4.0f, 2.0f, -3.0f},
				{0.0f, 2.0f, 3.0f},
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

				Entity pt = pECS->CreateEntity();
				pECS->AddComponent<PositionComponent>(pt, { startPosition[i], true });
				pECS->AddComponent<ScaleComponent>(pt, { glm::vec3(0.4f), true });
				pECS->AddComponent<RotationComponent>(pt, { glm::identity<glm::quat>(), true });
				pECS->AddComponent<PointLightComponent>(pt, pointLights[i]);
				pECS->AddComponent<MeshComponent>(pt, sphereMeshComp);
			}
		}
	}

	//Mirrors
	{
		MaterialProperties mirrorProperties = {};
		mirrorProperties.Roughness = 0.0f;

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

		pECS->AddComponent<PositionComponent>(entity, { {0.0f, 3.0f, -7.0f}, true });
		pECS->AddComponent<RotationComponent>(entity, { glm::toQuat(glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))), true });
		pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(1.5f), true });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
	}
}

void PlaySessionState::Tick(LambdaEngine::Timestamp)
{}
