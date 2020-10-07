#include "States/SandboxState.h"

#include "Resources/ResourceManager.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineConfig.h"

#include "Game/ECS/Components/Audio/AudibleComponent.h"
#include "Game/ECS/Components/Misc/Components.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Systems/TrackSystem.h"

#include "Input/API/Input.h"

#include "Math/Random.h"

#include "Physics/PhysicsSystem.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/Core/API/GraphicsTypes.h"

#include "Math/Random.h"

#include "GUI/GUITest.h"

#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

using namespace LambdaEngine;

SandboxState::SandboxState()
{

}

SandboxState::SandboxState(LambdaEngine::State* pOther) : LambdaEngine::State(pOther)
{
}

SandboxState::~SandboxState()
{
	if (m_GUITest.GetPtr() != nullptr)
	{
		int32 ref = m_GUITest->GetNumReferences();

		m_GUITest.Reset();
		m_View.Reset();
	}

	// Remove System
}

void SandboxState::Init()
{
	m_GUITest	= *new GUITest("Test.xaml");
	m_View		= Noesis::GUI::CreateView(m_GUITest);
	LambdaEngine::GUIApplication::SetView(m_View);

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SandboxState::OnKeyPressed);

	// Create Camera
	{
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		const CameraDesc cameraDesc =
		{
			.Position	= { 0.0f, 2.0f, 5.0f },
			.FOVDegrees	= EngineConfig::GetFloatProperty("CameraFOV"),
			.Width		= (float32)window->GetWidth(),
			.Height		= (float32)window->GetHeight(),
			.NearPlane	= EngineConfig::GetFloatProperty("CameraNearPlane"),
			.FarPlane	= EngineConfig::GetFloatProperty("CameraFarPlane")
		};
		Entity e = CreateFPSCameraEntity(cameraDesc);
	}

	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

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
			const StaticCollisionInfo collisionCreateInfo =
			{
				.Entity			= entity,
				.Position		= pECS->AddComponent<PositionComponent>(entity, { true, position }),
				.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, scale }),
				.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
				.Mesh			= pECS->AddComponent<MeshComponent>(entity, meshComponent),
				.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
				.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
			};

			pPhysicsSystem->CreateCollisionTriangleMesh(collisionCreateInfo);
		}
	}

	// Robot
	{
		TArray<GUID_Lambda> animations;
		const uint32 robotGUID			= ResourceManager::LoadMeshFromFile("Robot/Standard Walk.fbx", animations);
		const uint32 robotAlbedoGUID	= ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		const uint32 robotNormalGUID	= ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

		MaterialProperties materialProperties;
		materialProperties.Albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		materialProperties.Roughness = 1.0f;
		materialProperties.Metallic = 1.0f;

		const uint32 robotMaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Robot Material",
			robotAlbedoGUID,
			robotNormalGUID,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			materialProperties);

		MeshComponent robotMeshComp = {};
		robotMeshComp.MeshGUID = robotGUID;
		robotMeshComp.MaterialGUID = robotMaterialGUID;

		AnimationComponent robotAnimationComp = {};
		robotAnimationComp.AnimationGUID	= animations[0];
		robotAnimationComp.PlaybackSpeed	= 1.0f;
		robotAnimationComp.IsLooping		= false;
		// TODO: Safer way than getting the raw pointer (GUID for skeletons?)
		robotAnimationComp.Pose.pSkeleton	= ResourceManager::GetMesh(robotGUID)->pSkeleton;

		glm::vec3 position = glm::vec3(0.0f, 1.25f, -5.0f);
		glm::vec3 scale(0.01f);

		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		position = glm::vec3(0.0f, 1.25f, 0.0f);
		robotAnimationComp.IsLooping	= true;
		robotAnimationComp.NumLoops		= 10;

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		position = glm::vec3(-5.0f, 1.25f, 0.0f);
		robotAnimationComp.NumLoops = INFINITE_LOOPS;

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		position = glm::vec3(5.0f, 1.25f, 0.0f);

		robotAnimationComp.PlaybackSpeed *= -1.0f;

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		// Audio
		GUID_Lambda soundGUID = ResourceManager::LoadSoundEffectFromFile("halo_theme.wav");
		ISoundInstance3D* pSoundInstance = new SoundInstance3DFMOD(AudioAPI::GetDevice());
		const SoundInstance3DDesc desc =
		{
			.pName			= "RobotSoundInstance",
			.pSoundEffect	= ResourceManager::GetSoundEffect(soundGUID),
			.Flags			= FSoundModeFlags::SOUND_MODE_NONE,
			.Position		= position,
			.Volume			= 0.03f
		};

		pSoundInstance->Init(&desc);
		pECS->AddComponent<AudibleComponent>(entity, { pSoundInstance });
	}

	//Sphere Grid
	{
		const uint32 sphereMeshGUID	= ResourceManager::LoadMeshFromFile("sphere.obj");
		const uint32 gridRadius		= 5;
		for (uint32 y = 0; y < gridRadius; y++)
		{
			float32 roughness = y / float32(gridRadius - 1);

			for (uint32 x = 0; x < gridRadius; x++)
			{
				float32 metallic = x / float32(gridRadius - 1);

				MaterialProperties materialProperties;
				materialProperties.Albedo		= glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				materialProperties.Roughness	= roughness;
				materialProperties.Metallic		= metallic;

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
				const StaticCollisionInfo collisionCreateInfo = {
					.Entity			= entity,
					.Position		= pECS->AddComponent<PositionComponent>(entity, { true, position }),
					.Scale			= pECS->AddComponent<ScaleComponent>(entity, { true, scale }),
					.Rotation		= pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() }),
					.Mesh			= pECS->AddComponent<MeshComponent>(entity, sphereMeshComp),
					.CollisionGroup	= FCollisionGroup::COLLISION_GROUP_STATIC,
					.CollisionMask	= ~FCollisionGroup::COLLISION_GROUP_STATIC // Collide with any non-static object
				};

				pPhysicsSystem->CreateCollisionSphere(collisionCreateInfo);
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
		/*{
			constexpr uint32 POINT_LIGHT_COUNT = 3;
			const PointLightComponent pointLights[3] =
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

			const float32 PI = glm::pi<float>();
			const float32 RADIUS = 3.0f;
			for (uint32 i = 0; i < POINT_LIGHT_COUNT; i++)
			{
				float32 positive = std::powf(-1.0, i);

				glm::vec3 color = pointLights[i % 3].ColorIntensity;
				MaterialProperties materialProperties;
				materialProperties.Albedo		= glm::vec4(color, 1.0f);
				materialProperties.Roughness	= 0.1f;
				materialProperties.Metallic		= 0.1f;

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
				pECS->AddComponent<PositionComponent>(pt, { true, startPosition[i % 3] });
				pECS->AddComponent<ScaleComponent>(pt, { true, glm::vec3(0.4f) });
				pECS->AddComponent<RotationComponent>(pt, { true, glm::identity<glm::quat>() });
				pECS->AddComponent<PointLightComponent>(pt, pointLights[i % 3]);
				pECS->AddComponent<MeshComponent>(pt, sphereMeshComp);
			}
		}*/
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
		pECS->AddComponent<PositionComponent>(entity, { true, {0.0f, 3.0f, -7.0f} });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::toQuat(glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))) });
		pECS->AddComponent<ScaleComponent>(entity, { true, glm::vec3(1.5f) });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
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

void SandboxState::Tick(LambdaEngine::Timestamp delta)
{
	// Update State specfic objects

	//constexpr uint32 MAX_ENTITIES =  3;
	//static float timer = 0.0f;
	//static uint32 id = 0U;
	//static Entity entities[MAX_ENTITIES];
	//static bool remove = true;
	//timer += delta.AsSeconds();

	//if (id  < 3 && timer > 0.5)
	//{
	//	timer = 0.f;
	//	uint32 index = id % MAX_ENTITIES;

	//	if (index == 0)
	//		remove = !remove;

	//	if (!remove)
	//	{
	//		ECSCore* pECS = ECSCore::GetInstance();
	//		const uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

	//		const PointLightComponent pointLights = { .ColorIntensity = {index % 3, (index + 1U) % 3, (index + 2U) % 3, 25.0f}, .FarPlane = 20.0f };
	//		const glm::vec3 startPosition = { 0.0f, 2.0f, 3.0f - float(index) };

	//		const float32 PI = glm::pi<float>();
	//		const float32 RADIUS = 3.0f;
	//	
	//		entities[index] = pECS->CreateEntity();
	//		pECS->AddComponent<PositionComponent>(entities[index], { true, startPosition });
	//		pECS->AddComponent<ScaleComponent>(entities[index], { true, glm::vec3(0.4f) });
	//		pECS->AddComponent<RotationComponent>(entities[index], { true, glm::identity<glm::quat>() });
	//		pECS->AddComponent<PointLightComponent>(entities[index], pointLights);
	//	}
	//	else
	//	{
	//		ECSCore::GetInstance()->RemoveEntity(entities[index]);
	//	}
	//	id++;
	//}
}

bool SandboxState::OnKeyPressed(const LambdaEngine::KeyPressedEvent& event)
{
	using namespace LambdaEngine;


	constexpr uint32 MAX_ENTITIES = 10;
	static float timer = 0.0f;
	static uint32 id = 0U;
	static Entity entities[MAX_ENTITIES];
	static bool remove = true;
	//timer += delta.AsSeconds();

	if (event.Key == EKey::KEY_SPACE)
	{
		timer = 0.f;
		uint32 index = id % MAX_ENTITIES;

		if (index == 0)
			remove = !remove;

		if (!remove)
		{
			ECSCore* pECS = ECSCore::GetInstance();
			const uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

			const PointLightComponent pointLights = { .ColorIntensity = {index % 3, (index + 1U) % 3, (index + 2U) % 3, 25.0f}, .FarPlane = 20.0f };
			const glm::vec3 startPosition = { 0.0f, 2.0f, 3.0f - float(index) };

			const float32 PI = glm::pi<float>();
			const float32 RADIUS = 3.0f;

			glm::vec3 color = { 1.0f, 1.0f, 1.0f };
			MaterialProperties materialProperties;
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

			entities[index] = pECS->CreateEntity();
			pECS->AddComponent<PositionComponent>(entities[index], { true, startPosition });
			pECS->AddComponent<ScaleComponent>(entities[index], { true, glm::vec3(0.4f) });
			pECS->AddComponent<RotationComponent>(entities[index], { true, glm::identity<glm::quat>() });
			pECS->AddComponent<PointLightComponent>(entities[index], pointLights);

			pECS->AddComponent<MeshComponent>(entities[index], sphereMeshComp);
		}
		else
		{
			ECSCore::GetInstance()->RemoveEntity(entities[index]);
		}
		id++;
	}
	

	return true;
}
