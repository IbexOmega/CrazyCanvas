#include "States/SandboxState.h"

#include "Application/API/CommonApplication.h"
#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/MeshPaintComponent.h"
#include "Game/ECS/Components/Misc/MaskComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Input/API/Input.h"

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

	// Load scene
	{
		TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("sponza/sponza.obj", meshComponents);

		const glm::vec3 position(0.0f, 0.0f, 0.0f);
		const glm::vec3 scale(0.01f);

		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = ECSCore::GetInstance()->CreateEntity();
			pECS->AddComponent<PositionComponent>(entity, { position, true });
			pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
			pECS->AddComponent<ScaleComponent>(entity, { scale, true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
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

		constexpr const uint32 NUM_MIRRORS = 6;
		for (uint32 i = 0; i < NUM_MIRRORS; i++)
		{
			Entity entity = ECSCore::GetInstance()->CreateEntity();

			float32 sign = pow(-1.0f, i % 2);
			pECS->AddComponent<PositionComponent>(entity, { glm::vec3(3.0f * (float32(i / 2) - float32(NUM_MIRRORS) / 2.0f), 2.0f, 1.5f * sign), true });
			pECS->AddComponent<RotationComponent>(entity, { glm::toQuat(glm::rotate(glm::identity<glm::mat4>(), glm::radians(-sign * 90.0f), glm::vec3(1.0f, 0.0f, 0.0f))), true });
			pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(1.0f), true });
			pECS->AddComponent<MeshComponent>(entity, meshComponent);
		}
	}

	// Plane for painting.
	{
		MaterialProperties mirrorProperties = {};
		mirrorProperties.Roughness = 1.f;
		mirrorProperties.Metallic = 0.f;

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

		MeshPaintComponent meshPaintComponent;
		const uint32 width = 512;
		const uint32 height = 512;
		char* data = DBG_NEW char[width*height*4];
		memset(data, 0, width * height * 4);
		meshPaintComponent.UnwrappedTexture = ResourceManager::LoadTextureFromMemory("PlaneUnwrappedTexture_1", data, width, height, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET, false);
		SAFEDELETE_ARRAY(data);

		MaskComponent maskComponent;
		maskComponent.Mask = 0x00000000;

		Entity entity = ECSCore::GetInstance()->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { {0.f, 0.f, 0.f}, true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::mat4>(), true });
		pECS->AddComponent<ScaleComponent>(entity, { glm::vec3(1.0f), true });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
		pECS->AddComponent<MeshPaintComponent>(entity, meshPaintComponent);
		pECS->AddComponent<MaskComponent>(entity, maskComponent);
	}
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

void SandboxState::Tick(LambdaEngine::Timestamp)
{
	// Update State specfic objects
}
