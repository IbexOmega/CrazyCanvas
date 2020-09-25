#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"
#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
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

	// Load scene
	TArray<MeshComponent> meshComponents;
	ResourceManager::LoadSceneFromFile("Map/scene.obj", meshComponents);

	const glm::vec3 position(0.0f, 0.0f, 0.0f);
	const glm::vec3 scale(1.0f);

	ECSCore* pECS = ECSCore::GetInstance();
	for (const MeshComponent& meshComponent : meshComponents)
	{
		const Entity entity = ECSCore::GetInstance()->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { position, true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
	}
}

void PlaySessionState::Tick(LambdaEngine::Timestamp)
{}
