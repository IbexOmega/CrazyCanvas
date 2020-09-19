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
			ECSCore::GetInstance()->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
			ECSCore::GetInstance()->AddComponent<ScaleComponent>(entity, { scale, true });
			ECSCore::GetInstance()->AddComponent<MeshComponent>(entity, meshComponents[i]);
		}
	}

	GUID_Lambda sphereMeshGUID = ResourceManager::LoadMeshFromFile("triangle.obj");
	GUID_Lambda Material = ResourceManager::LoadMaterialFromMemory(
		"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_NORMAL_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		materialProperties);

	Entity e0 = ECSCore::GetInstance()->CreateEntity();
	ECSCore::GetInstance()->AddComponent<PositionComponent>(e0, { {1.0f, 0.0f, 0.0f}, true });
	ECSCore::GetInstance()->AddComponent<ScaleComponent>(e0, { {1.0f, 1.0f, 1.0f}, true });
	ECSCore::GetInstance()->AddComponent<RotationComponent>(e0, { glm::identity<glm::quat>(), true });
	ECSCore::GetInstance()->AddComponent<MeshComponent>(e0, MeshComponent{ .MeshGUID = sphereMeshGUID, .MaterialGUID = Material });

	Entity e1 = ECSCore::GetInstance()->CreateEntity();
	ECSCore::GetInstance()->AddComponent<PositionComponent>(e1, { {-1.0f, 0.0f, 0.0f}, true });
	ECSCore::GetInstance()->AddComponent<ScaleComponent>(e1, { {1.0f, 1.0f, 1.0f}, true });
	ECSCore::GetInstance()->AddComponent<RotationComponent>(e1, { glm::identity<glm::quat>(), true });
	ECSCore::GetInstance()->AddComponent<MeshComponent>(e1, MeshComponent{ .MeshGUID = sphereMeshGUID, .MaterialGUID = Material });

	Entity e2 = ECSCore::GetInstance()->CreateEntity();
	ECSCore::GetInstance()->AddComponent<PositionComponent>(e2, { {0.0f, 0.0f, 1.0f}, true });
	ECSCore::GetInstance()->AddComponent<ScaleComponent>(e2, { {1.0f, 1.0f, 1.0f}, true });
	ECSCore::GetInstance()->AddComponent<RotationComponent>(e2, { glm::identity<glm::quat>(), true });
	ECSCore::GetInstance()->AddComponent<MeshComponent>(e2, MeshComponent{ .MeshGUID = sphereMeshGUID, .MaterialGUID = Material });

	Entity e3 = ECSCore::GetInstance()->CreateEntity();
	ECSCore::GetInstance()->AddComponent<PositionComponent>(e3, { {0.0f, 0.0f, -1.0f}, true });
	ECSCore::GetInstance()->AddComponent<ScaleComponent>(e3, { {1.0f, 1.0f, 1.0f}, true });
	ECSCore::GetInstance()->AddComponent<RotationComponent>(e3, { glm::identity<glm::quat>(), true });
	ECSCore::GetInstance()->AddComponent<MeshComponent>(e3, MeshComponent{ .MeshGUID = sphereMeshGUID, .MaterialGUID = Material });

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
