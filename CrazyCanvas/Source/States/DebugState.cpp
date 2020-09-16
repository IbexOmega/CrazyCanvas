#include "States/DebugState.h"
#include "Log/Log.h"

#include "Resources/ResourceManager.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

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
	RenderSystem::GetInstance().Init();

	Entity e0 = ECSCore::GetInstance()->CreateEntity();
	Entity e1 = ECSCore::GetInstance()->CreateEntity();

	MaterialProperties materialProperties;
	materialProperties.Albedo = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	materialProperties.Roughness = 0.1f;
	materialProperties.Metallic = 0.1f;

	GUID_Lambda sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
	GUID_Lambda Material = ResourceManager::LoadMaterialFromMemory(
		"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_NORMAL_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		GUID_TEXTURE_DEFAULT_COLOR_MAP,
		materialProperties);

	//StaticMeshComponent staticMeshComponent =
	//{
	//	{
	//		.MeshGUID = sphereMeshGUID,
	//		.MaterialGUID = Material,
	//	}
	//};

	//DynamicMeshComponent dynamicMeshComponent =
	//{
	//	{
	//		.MeshGUID = sphereMeshGUID,
	//		.MaterialGUID = Material,
	//	}
	//};

	ECSCore::GetInstance()->AddComponent<PositionComponent>(e0, { {0.0f, 0.0f, 0.0f} });
	ECSCore::GetInstance()->AddComponent<ScaleComponent>(e0, { {0.0f, 0.0f, 0.0f} });
	ECSCore::GetInstance()->AddComponent<RotationComponent>(e0, { glm::identity<glm::quat>() });
	ECSCore::GetInstance()->AddComponent<StaticMeshComponent>(e0, StaticMeshComponent{ {.MeshGUID = sphereMeshGUID, .MaterialGUID = Material} });

	ECSCore::GetInstance()->AddComponent<PositionComponent>(e1, { {0.0f, 0.0f, 0.0f} });
	ECSCore::GetInstance()->AddComponent<ScaleComponent>(e1, { {0.0f, 0.0f, 0.0f} });
	ECSCore::GetInstance()->AddComponent<RotationComponent>(e1, { glm::identity<glm::quat>() });
	ECSCore::GetInstance()->AddComponent<DynamicMeshComponent>(e1, DynamicMeshComponent{ {.MeshGUID = sphereMeshGUID, .MaterialGUID = Material} });


	// Load Scene SceneManager::Get("SceneName").Load()

	// Use HelperClass to create additional entities

	// EntityIndex index = HelperClass::CreatePlayer(
	
	// Entity e = ECSCore::Get().createEntity()
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
