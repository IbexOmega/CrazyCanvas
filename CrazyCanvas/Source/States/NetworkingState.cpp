#include "States/NetworkingState.h"

#include "Application/API/CommonApplication.h"
#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"
#include "Game/ECS/Components/Player/ControllableComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Input/API/Input.h"

void NetworkingState::Init()
{
	using namespace LambdaEngine;

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees = EngineConfig::GetFloatProperty("CameraFOV");
	cameraDesc.Width = window->GetWidth();
	cameraDesc.Height = window->GetHeight();
	cameraDesc.NearPlane = EngineConfig::GetFloatProperty("CameraNearPlane");
	cameraDesc.FarPlane = EngineConfig::GetFloatProperty("CameraFarPlane");
	cameraDesc.Position = glm::vec3(0.f, 3.f, 0.f);
	CreateFreeCameraEntity(cameraDesc);

	ECSCore* pECS = ECSCore::GetInstance();

	// Load scene
	{
		TArray<MeshComponent> meshComponents;
		LambdaEngine::TArray<LambdaEngine::LoadedDirectionalLight>	directionalLights;
		LambdaEngine::TArray<LambdaEngine::LoadedPointLight>		pointLights;
		LambdaEngine::TArray<LambdaEngine::SpecialObject>			specialObjects;

		SceneLoadDesc sceneLoadDesc = {};
		sceneLoadDesc.Filename = "Testing/Testing.obj";

		ResourceManager::LoadSceneFromFile(&sceneLoadDesc, meshComponents, directionalLights, pointLights, specialObjects);

		const glm::vec3 position(0.0f, 0.0f, 0.0f);
		const glm::vec3 scale(1.0f);

		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = pECS->CreateEntity();
			pECS->AddComponent<PositionComponent>(entity, { true, position });
			pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
			pECS->AddComponent<ScaleComponent>(entity, { true, scale });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
		}
	}

	//Create Player
	/*{
		MaterialProperties materialProperties = {};
		materialProperties.Roughness	= 0.1f;
		materialProperties.Metallic	= 0.0f;
		materialProperties.Albedo		= glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

		MeshComponent meshComponent;
		meshComponent.MeshGUID		= ResourceManager::LoadMeshFromFile("sphere.obj");
		meshComponent.MaterialGUID	= ResourceManager::LoadMaterialFromMemory(
			"Mirror Material",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { glm::vec3(1.0f, 1.0f, 0.0f), true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(1.0f), true });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
		pECS->AddComponent<NetworkComponent>(entity, {});
		pECS->AddComponent<ControllableComponent>(entity, {true});
	}*/
}

void NetworkingState::Tick(LambdaEngine::Timestamp)
{

}
