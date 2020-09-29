#include "States/SandboxState.h"

#include "Resources/ResourceManager.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "ECS/ECSCore.h"
#include "Engine/EngineConfig.h"

#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/MeshPaintComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Systems/TrackSystem.h"
#include "Input/API/Input.h"
#include "Math/Random.h"

#include "Math/Random.h"

#include "Physics/PhysicsSystem.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/EntityMaskManager.h"
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
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	// Create Camera
	{
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		CameraDesc cameraDesc = {};
		cameraDesc.FOVDegrees	= EngineConfig::GetFloatProperty("CameraFOV");
		cameraDesc.Position		= glm::vec3(0.0f, 2.0f, -2.0f);
		cameraDesc.Width		= window->GetWidth();
		cameraDesc.Height		= window->GetHeight();
		cameraDesc.NearPlane	= EngineConfig::GetFloatProperty("CameraNearPlane");
		cameraDesc.FarPlane		= EngineConfig::GetFloatProperty("CameraFarPlane");
		Entity e = CreateFreeCameraEntity(cameraDesc);
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

	{
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("cube.obj");

		MaterialProperties materialProperties;
		materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		materialProperties.Roughness = 0.5f;
		materialProperties.Metallic = 0.5f;

		MeshComponent sphereMeshComp = {};
		sphereMeshComp.MeshGUID = sphereMeshGUID;
		sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Default r: " + std::to_string(materialProperties.Roughness) + " m: " + std::to_string(materialProperties.Metallic),
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		glm::vec3 position(0.f, 0.f, -4.5f);
		glm::vec3 scale(1/5.f);
		glm::quat rotation = glm::identity<glm::quat>();

		MeshPaintComponent meshPaintComponent;
		const uint32 width = 512;
		const uint32 height = 512;
		char* data = DBG_NEW char[width * height * 4];
		memset(data, 0, width * height * 4);
		meshPaintComponent.UnwrappedTexture = ResourceManager::LoadTextureFromMemory("CubeUnwrappedTexture_1", data, width, height, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET, false);
		SAFEDELETE_ARRAY(data);

		m_Entity = pECS->CreateEntity();
		m_Entities.PushBack(m_Entity);
		pECS->AddComponent<PositionComponent>(m_Entity, { position, true });
		pECS->AddComponent<ScaleComponent>(m_Entity, { scale, true });
		pECS->AddComponent<RotationComponent>(m_Entity, { rotation, true });
		pECS->AddComponent<MeshComponent>(m_Entity, sphereMeshComp);
		pECS->AddComponent<MeshPaintComponent>(m_Entity, meshPaintComponent);

		DrawArgExtensionData drawArgExtensionData = {};
		drawArgExtensionData.TextureCount = 1;
		drawArgExtensionData.ppTextures[0] = ResourceManager::GetTexture(meshPaintComponent.UnwrappedTexture);
		drawArgExtensionData.ppTextureViews[0] = ResourceManager::GetTextureView(meshPaintComponent.UnwrappedTexture);
		//drawArgExtensionData.ppSamplers[0] = ;
		EntityMaskManager::AddExtensionToEntity(m_Entity, MeshPaintComponent::Type(), drawArgExtensionData);
	}

	{
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("quad.obj");

		MaterialProperties materialProperties;
		materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		materialProperties.Roughness = 1.f;
		materialProperties.Metallic = 0.f;

		MeshComponent sphereMeshComp = {};
		sphereMeshComp.MeshGUID = sphereMeshGUID;
		sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Default r: " + std::to_string(materialProperties.Roughness) + " m: " + std::to_string(materialProperties.Metallic),
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		glm::vec3 position(0.f, -1.f, -4.f);
		glm::vec3 scale(40.f);
		glm::quat rotation = glm::rotate(glm::identity<glm::quat>(), glm::radians(-180.f), glm::vec3(0.f, 1.f, 0.f));

		MeshPaintComponent meshPaintComponent;
		const uint32 width = 512;
		const uint32 height = 512;
		char* data = DBG_NEW char[width * height * 4];
		memset(data, 0, width* height * 4);
		meshPaintComponent.UnwrappedTexture = ResourceManager::LoadTextureFromMemory("PlaneUnwrappedTexture_1", data, width, height, EFormat::FORMAT_R8G8B8A8_UNORM, FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_RENDER_TARGET, false);
		SAFEDELETE_ARRAY(data);

		Entity entity = pECS->CreateEntity();
		m_Entities.PushBack(entity);
		pECS->AddComponent<PositionComponent>(entity, { position, true });
		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
		pECS->AddComponent<RotationComponent>(entity, { rotation, true });
		pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, meshPaintComponent);

		DrawArgExtensionData drawArgExtensionData = {};
		drawArgExtensionData.TextureCount = 1;
		drawArgExtensionData.ppTextures[0] = ResourceManager::GetTexture(meshPaintComponent.UnwrappedTexture);
		drawArgExtensionData.ppTextureViews[0] = ResourceManager::GetTextureView(meshPaintComponent.UnwrappedTexture);
		//drawArgExtensionData.ppSamplers[0] = ;
		EntityMaskManager::AddExtensionToEntity(entity, MeshPaintComponent::Type(), drawArgExtensionData);
	}

	//Scene
	/*{
		TArray<MeshComponent> meshComponents;
		ResourceManager::LoadSceneFromFile("Prototype/PrototypeScene.dae", meshComponents);

		const glm::vec3 position(0.0f, 0.0f, 0.0f);
		const glm::vec3 scale(1.0f);

		for (const MeshComponent& meshComponent : meshComponents)
		{
			Entity entity = ECSCore::GetInstance()->CreateEntity();
			CollisionCreateInfo collisionCreateInfo = {
				.Entity			= entity,
				.Position		= pECS->AddComponent<PositionComponent>(entity, { position, true }),
				.Scale			= pECS->AddComponent<ScaleComponent>(entity, { scale, true }),
				.Rotation		= pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true }),
				.Mesh			= pECS->AddComponent<MeshComponent>(entity, meshComponent),
				.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
				.CollisionMask	= FCollisionGroup::COLLISION_GROUP_STATIC
			};

			pPhysicsSystem->CreateCollisionTriangleMesh(collisionCreateInfo);

			m_Entities.PushBack(entity);
		}
	}*/

	// Simple Scene (One object, One texture)
	{
		/*
		uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("bunny.obj");

		MaterialProperties materialProperties;
		materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		materialProperties.Roughness = 0.5f;
		materialProperties.Metallic = 0.5f;

		MeshComponent sphereMeshComp = {};
		sphereMeshComp.MeshGUID = sphereMeshGUID;
		sphereMeshComp.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Default r: " + std::to_string(materialProperties.Roughness) + " m: " + std::to_string(materialProperties.Metallic),
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		glm::vec3 position(0.f, 0.f, 0.0f);
		glm::vec3 scale(1.f);

		Entity entity = pECS->CreateEntity();
		m_Entities.PushBack(entity);
		pECS->AddComponent<PositionComponent>(entity, { position, true });
		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<MeshComponent>(entity, sphereMeshComp);
		*/
	}

	// Robot
	/*{
		const uint32 robotGUID			= ResourceManager::LoadMeshFromFile("Robot/Standard Walk.fbx");
		const uint32 robotAlbedoGUID	= ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		const uint32 robotNormalGUID	= ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		
		MaterialProperties materialProperties;
		materialProperties.Albedo		= glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		materialProperties.Roughness	= 1.0f;
		materialProperties.Metallic		= 1.0f;
		
		const uint32 robotMaterialGUID	= ResourceManager::LoadMaterialFromMemory(
			"Robot Material",
			robotAlbedoGUID,
			robotNormalGUID,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);
		
		MeshComponent robotMeshComp = {};
		robotMeshComp.MeshGUID		= robotGUID;
		robotMeshComp.MaterialGUID	= robotMaterialGUID;

		glm::vec3 position(0.0f, 1.25f, 0.0f);
		glm::vec3 scale(0.01f);

		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { position, true });
		pECS->AddComponent<ScaleComponent>(entity, { scale, true });
		pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true });
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		m_Entities.PushBack(entity);
	}*/

	//Sphere Grid
	{
		/*uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

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

				MeshComponent sphereMeshComp = { };
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
				m_Entities.PushBack(entity);
				CollisionCreateInfo collisionCreateInfo = 
				{
					.Entity			= entity,
					.Position		= pECS->AddComponent<PositionComponent>(entity, { position, true }),
					.Scale			= pECS->AddComponent<ScaleComponent>(entity, { scale, true }),
					.Rotation		= pECS->AddComponent<RotationComponent>(entity, { glm::identity<glm::quat>(), true }),
					.Mesh			= pECS->AddComponent<MeshComponent>(entity, sphereMeshComp),
					.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
					.CollisionMask	= FCollisionGroup::COLLISION_GROUP_STATIC
				};

				pPhysicsSystem->CreateCollisionSphere(collisionCreateInfo);

				glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), position);
				transform *= glm::toMat4(glm::identity<glm::quat>());
				transform = glm::scale(transform, scale);
			}
		}*/

		//// Directional Light
		{
			/*m_DirLight = ECSCore::GetInstance()->CreateEntity();
			ECSCore::GetInstance()->AddComponent<PositionComponent>(m_DirLight, { {0.f, 0.f, 0.f}, true });
			ECSCore::GetInstance()->AddComponent<RotationComponent>(m_DirLight, { glm::quatLookAt(glm::normalize(-g_DefaultRight - g_DefaultUp), g_DefaultUp), true });
			ECSCore::GetInstance()->AddComponent<DirectionalLightComponent>(m_DirLight,
				DirectionalLightComponent{
					.ColorIntensity = {1.0f, 1.0f, 1.0f, 15.0f},
					.frustumWidth = 20.0f,
					.frustumHeight = 20.0f,
					.frustumZNear = -40.0f,
					.frustumZFar = 10.0f,
				}
			);*/
		}

		// Add PointLights
		/*{
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
		}*/
	}

	//Mirrors
	/*{
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
	}*/
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
	// Update State specfic objects
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
