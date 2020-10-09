#include "States/SandboxState.h"

#include "Resources/ResourceManager.h"
#include "Resources/AnimationGraph.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "Debug/Profiler.h"

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
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Systems/TrackSystem.h"
#include "Game/GameConsole.h"

#include "Input/API/Input.h"

#include "Math/Random.h"


#include "Rendering/Core/API/GraphicsTypes.h"
#include "Rendering/ImGuiRenderer.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphEditor.h"

#include "Math/Random.h"

#include "GUI/GUITest.h"

#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#include <imgui.h>

#include "World/LevelManager.h"
#include "World/Level.h"

using namespace LambdaEngine;

SandboxState::~SandboxState()
{
    EventQueue::UnregisterEventHandler<KeyPressedEvent>(EventHandler(this, &SandboxState::OnKeyPressed));

	if (m_GUITest.GetPtr() != nullptr)
	{
		m_GUITest.Reset();
		m_View.Reset();
	}

	SAFEDELETE(m_pRenderGraphEditor);
	SAFEDELETE(m_pLevel);
}

void SandboxState::Init()
{
	// Create Systems
	TrackSystem::GetInstance().Init();
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SandboxState::OnKeyPressed);
	ECSCore* pECS = ECSCore::GetInstance();
	PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

	m_RenderGraphWindow = EngineConfig::GetBoolProperty("ShowRenderGraph");
	m_ShowDemoWindow = EngineConfig::GetBoolProperty("ShowDemo");
	m_DebuggingWindow = EngineConfig::GetBoolProperty("Debugging");

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
		CreateFPSCameraEntity(cameraDesc);
	}

	// Scene
	{
		m_pLevel = LevelManager::LoadLevel(0);
	}

	// Robot
	{
		TArray<GUID_Lambda> animations;
		const uint32 robotGUID = ResourceManager::LoadMeshFromFile("Robot/Rumba Dancing.fbx", animations);
		const uint32 robotAlbedoGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);
		const uint32 robotNormalGUID = ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true);

		TArray<GUID_Lambda> running		= ResourceManager::LoadAnimationsFromFile("Robot/Running.fbx");
		TArray<GUID_Lambda> thriller	= ResourceManager::LoadAnimationsFromFile("Robot/Thriller.fbx");

		MaterialProperties materialProperties;
		materialProperties.Albedo		= glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		materialProperties.Roughness	= 1.0f;
		materialProperties.Metallic		= 1.0f;

		const uint32 robotMaterialGUID = ResourceManager::LoadMaterialFromMemory(
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

		AnimationComponent robotAnimationComp = {};
		robotAnimationComp.Graph			= AnimationGraph(AnimationState("thriller", thriller[0]));
		robotAnimationComp.Pose.pSkeleton	= ResourceManager::GetMesh(robotGUID)->pSkeleton; // TODO: Safer way than getting the raw pointer (GUID for skeletons?)

		glm::vec3 position = glm::vec3(0.0f, 0.9f, -2.5f);
		glm::vec3 scale(0.01f);

		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
	
		position = glm::vec3(0.0f, 1.0f, 0.0f);
		robotAnimationComp.Graph = AnimationGraph(AnimationState("walking", animations[0]));

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		position = glm::vec3(-3.5f, 0.9f, 0.0f);
		robotAnimationComp.Graph = AnimationGraph(AnimationState("running", running[0]));

		entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);

		position = glm::vec3(3.5f, 0.9f, 0.0f);

		AnimationGraph animationGraph;
		animationGraph.AddState(AnimationState("running", running[0]));
		animationGraph.AddState(AnimationState("walking", animations[0]));
		animationGraph.AddTransition(Transition("running", "walking", 0.2));
		animationGraph.AddTransition(Transition("walking", "running", 0.5));
		robotAnimationComp.Graph = animationGraph;

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
				.pName = "RobotSoundInstance",
				.pSoundEffect = ResourceManager::GetSoundEffect(soundGUID),
				.Flags = FSoundModeFlags::SOUND_MODE_NONE,
				.Position = position,
				.Volume = 0.03f
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
		{
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
		pECS->AddComponent<PositionComponent>(entity, { true, {0.0f, 3.0f, -7.0f} });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::toQuat(glm::rotate(glm::identity<glm::mat4>(), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))) });
		pECS->AddComponent<ScaleComponent>(entity, { true, glm::vec3(1.5f) });
		pECS->AddComponent<MeshComponent>(entity, meshComponent);
	}

	if constexpr (IMGUI_ENABLED)
	{
		ImGui::SetCurrentContext(ImGuiRenderer::GetImguiContext());

		m_pRenderGraphEditor = DBG_NEW RenderGraphEditor();
		m_pRenderGraphEditor->InitGUI();	//Must Be called after Renderer is initialized
	}

	ConsoleCommand cmd1;
	cmd1.Init("render_graph", true);
	cmd1.AddArg(Arg::EType::BOOL);
	cmd1.AddDescription("Activate/Deactivate rendergraph window.\n\t'render_graph true'");
	GameConsole::Get().BindCommand(cmd1, [&, this](GameConsole::CallbackInput& input)->void {
		m_RenderGraphWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand cmd2;
	cmd2.Init("imgui_demo", true);
	cmd2.AddArg(Arg::EType::BOOL);
	cmd2.AddDescription("Activate/Deactivate demo window.\n\t'imgui_demo true'");
	GameConsole::Get().BindCommand(cmd2, [&, this](GameConsole::CallbackInput& input)->void {
		m_ShowDemoWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand cmd3;
	cmd3.Init("show_debug_window", false);
	cmd3.AddArg(Arg::EType::BOOL);
	cmd3.AddDescription("Activate/Deactivate debugging window.\n\t'show_debug_window true'");
	GameConsole::Get().BindCommand(cmd3, [&, this](GameConsole::CallbackInput& input)->void {
		m_DebuggingWindow = input.Arguments.GetFront().Value.Boolean;
		});

	ConsoleCommand showTextureCMD;
	showTextureCMD.Init("debug_texture", true);
	showTextureCMD.AddArg(Arg::EType::BOOL);
	showTextureCMD.AddFlag("t", Arg::EType::STRING);
	showTextureCMD.AddFlag("ps", Arg::EType::STRING);
	showTextureCMD.AddDescription("Show a texture resource which is used in the RenderGraph");
	GameConsole::Get().BindCommand(showTextureCMD, [&, this](GameConsole::CallbackInput& input)->void
		{
			m_ShowTextureDebuggingWindow = input.Arguments.GetFront().Value.Boolean;

			auto textureNameIt				= input.Flags.find("t");
			auto shaderNameIt				= input.Flags.find("ps");
			m_TextureDebuggingName			= textureNameIt != input.Flags.end() ? textureNameIt->second.Arg.Value.String : "";
			m_TextureDebuggingShaderGUID	= shaderNameIt != input.Flags.end() ? ResourceManager::GetShaderGUID(shaderNameIt->second.Arg.Value.String) : GUID_NONE;
		});
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
	m_pRenderGraphEditor->Update();
	LambdaEngine::Profiler::Tick(delta);

	if constexpr (IMGUI_ENABLED)
	{
		RenderImgui();
	}
}

void SandboxState::OnRenderGraphRecreate(LambdaEngine::RenderGraph* pRenderGraph)
{
	using namespace LambdaEngine;

	Sampler* pNearestSampler				= Sampler::GetNearestSampler();

	GUID_Lambda blueNoiseID = ResourceManager::GetTextureGUID("Blue Noise Texture");

	Texture* pBlueNoiseTexture				= ResourceManager::GetTexture(blueNoiseID);
	TextureView* pBlueNoiseTextureView		= ResourceManager::GetTextureView(blueNoiseID);

	ResourceUpdateDesc blueNoiseUpdateDesc = {};
	blueNoiseUpdateDesc.ResourceName								= "BLUE_NOISE_LUT";
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures			= &pBlueNoiseTexture;
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews		= &pBlueNoiseTextureView;
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers			= &pNearestSampler;

	pRenderGraph->UpdateResource(&blueNoiseUpdateDesc);

	GUID_Lambda cubemapTexID = ResourceManager::GetTextureGUID("Cubemap Texture");

	Texture* pCubeTexture			= ResourceManager::GetTexture(cubemapTexID);
	TextureView* pCubeTextureView	= ResourceManager::GetTextureView(cubemapTexID);

	ResourceUpdateDesc cubeTextureUpdateDesc = {};
	cubeTextureUpdateDesc.ResourceName = "SKYBOX";
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures		= &pCubeTexture;
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews	= &pCubeTextureView;
	cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pNearestSampler;

	pRenderGraph->UpdateResource(&cubeTextureUpdateDesc);
}

void SandboxState::RenderImgui()
{
	using namespace LambdaEngine;

	ImGuiRenderer::Get().DrawUI([&]()
	{
		if (m_RenderGraphWindow)
			m_pRenderGraphEditor->RenderGUI();

		if (m_ShowDemoWindow)
			ImGui::ShowDemoWindow();

		if (m_DebuggingWindow)
		{
			Profiler::Render();
		}

		if (m_ShowTextureDebuggingWindow)
		{
			if (ImGui::Begin("Texture Debugging"))
			{
				if (!m_TextureDebuggingName.empty())
				{
					static ImGuiTexture texture = {};
					texture.ResourceName		= m_TextureDebuggingName;
					texture.PixelShaderGUID		= m_TextureDebuggingShaderGUID;

					ImGui::Image(&texture, ImGui::GetWindowSize());
				}
			}

			ImGui::End();
		}
	});
}

bool SandboxState::OnKeyPressed(const LambdaEngine::KeyPressedEvent& event)
{
	using namespace LambdaEngine;

	if (!IsEventOfType<KeyPressedEvent>(event))
	{
		return false;
	}

	if (event.IsRepeat)
	{
		return false;
	}

	static bool geometryAudioActive = true;
	static bool reverbSphereActive = true;

	if (event.Key == EKey::KEY_5)
	{
		EventQueue::SendEvent(ShaderRecompileEvent());
		EventQueue::SendEvent(PipelineStateRecompileEvent());
	}

	return true;
}
