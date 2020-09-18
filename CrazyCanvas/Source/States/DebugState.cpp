#include "States/DebugState.h"
#include "Log/Log.h"

#include "Resources/ResourceManager.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

using namespace LambdaEngine;

DebugState::DebugState()
{

}

DebugState::DebugState(LambdaEngine::State* pOther) : LambdaEngine::State(pOther)
{
}

DebugState::~DebugState()
{
	// Remove System
}

void DebugState::Init()
{
	// Create Systems
	MaterialProperties materialProperties;
	materialProperties.Albedo = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	materialProperties.Roughness = 0.1f;
	materialProperties.Metallic = 0.1f;

	ECSCore* pECS = ECSCore::GetInstance();
	//Scene
	{
		TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("sponza/sponza.obj", meshComponents);

		glm::vec3 position(0.0f, 0.0f, 0.0f);
		glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
		glm::vec3 scale(0.01f);

		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = pECS->CreateEntity();
			pECS->AddComponent<PositionComponent>(entity, { position, true });
			pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
			pECS->AddComponent<ScaleComponent>(entity, { scale, true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
			pECS->AddComponent<StaticComponent>(entity, StaticComponent());
		}
	}

	GUID_Lambda sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
	GUID_Lambda Material = ResourceManager::LoadMaterialFromMemory(
		"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_NORMAL_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		materialProperties);

	Entity e0 = pECS->CreateEntity();
	Entity e1 = pECS->CreateEntity();

	pECS->AddComponent<PositionComponent>(e0, { {0.0f, 0.0f, 0.0f}, true });
	pECS->AddComponent<ScaleComponent>(e0, { {0.0f, 0.0f, 0.0f}, true });
	pECS->AddComponent<RotationComponent>(e0, { glm::identity<glm::quat>(), true });
	pECS->AddComponent<MeshComponent>(e0, MeshComponent{ .MeshGUID = sphereMeshGUID, .MaterialGUID = Material });
	pECS->AddComponent<StaticComponent>(e0, StaticComponent() );

	pECS->AddComponent<PositionComponent>(e1, { {0.0f, 0.0f, 0.0f}, true });
	pECS->AddComponent<ScaleComponent>(e1, { {0.0f, 0.0f, 0.0f}, true });
	pECS->AddComponent<RotationComponent>(e1, { glm::identity<glm::quat>(), true });
	pECS->AddComponent<MeshComponent>(e1, MeshComponent{ .MeshGUID = sphereMeshGUID, .MaterialGUID = Material });
	pECS->AddComponent<DynamicComponent>(e1, DynamicComponent() );

	// Load Scene SceneManager::Get("SceneName").Load()

	// Use HelperClass to create additional entities

	// EntityIndex index = HelperClass::CreatePlayer(
}

void DebugState::Resume()
{
	// Unpause System

	// Reload Page
}

void DebugState::Pause()
{
	// Pause System

	// Unload Page
}

void DebugState::Tick(float dt)
{
	// Update State specfic objects
}
