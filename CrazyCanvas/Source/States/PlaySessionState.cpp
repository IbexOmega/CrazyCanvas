#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"
#include "ECS/ECSCore.h"
#include "Game/Camera.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Input/API/Input.h"

PlaySessionState::PlaySessionState()
	:m_pCamera(nullptr)
{}

void PlaySessionState::Init()
{
	using namespace LambdaEngine;
	Input::Disable();

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= 90.0f;
	cameraDesc.Width		= window->GetWidth();
	cameraDesc.Height		= window->GetHeight();
	cameraDesc.NearPlane	= 0.001f;
	cameraDesc.FarPlane		= 1000.0f;

	m_pCamera.reset(DBG_NEW Camera());
	m_pCamera->Init(cameraDesc);
	m_pCamera->Update();

	RenderSystem::GetInstance().SetCamera(m_pCamera.get());

	// Load scene
	TArray<MeshComponent> meshComponents;
	ResourceManager::LoadSceneFromFile("sponza/sponza.obj", meshComponents);

	glm::vec3 position(0.0f, 0.0f, 0.0f);
	glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
	glm::vec3 scale(0.01f);

	ECSCore* pECS = ECSCore::GetInstance();
	for (const MeshComponent& meshComponent : meshComponents)
	{
		Entity entity = ECSCore::GetInstance()->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { position, true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
		pECS->AddComponent<StaticComponent>(entity, StaticComponent());
	}
}

void PlaySessionState::Tick(float dt)
{
	m_pCamera->Update();
}
