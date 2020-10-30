#include "States/SandboxState.h"

#include "Resources/ResourceManager.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioAPI.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"

#include "Debug/Profiler.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
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
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Systems/TrackSystem.h"
#include "ECS/Systems/Player/WeaponSystem.h"
#include "Game/GameConsole.h"

#include "Input/API/Input.h"

#include "Math/Random.h"

#include "Rendering/Core/API/GraphicsTypes.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphEditor.h"
#include "Rendering/Animation/AnimationGraph.h"

#include "Math/Random.h"

#include "GUI/GUITest.h"

#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#include "World/LevelManager.h"
#include "World/LevelObjectCreator.h"

#include "Match/Match.h"

#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Rendering/EntityMaskManager.h"
#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/SingleplayerInitializer.h"

#include <imgui.h>

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
}

void SandboxState::Init()
{
	// Initialize event handlers
	m_AudioEffectHandler.Init();
	m_MeshPaintHandler.Init();
	m_MultiplayerClient.InitInternal();

	// Initialize Systems
	TrackSystem::GetInstance().Init();
	
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SandboxState::OnKeyPressed);

	m_RenderGraphWindow = EngineConfig::GetBoolProperty("ShowRenderGraph");
	m_ShowDemoWindow	= EngineConfig::GetBoolProperty("ShowDemo");
	m_DebuggingWindow	= EngineConfig::GetBoolProperty("Debugging");

	m_GUITest	= *new GUITest("Test.xaml");
	m_View		= Noesis::GUI::CreateView(m_GUITest);
	LambdaEngine::GUIApplication::SetView(m_View);

	ECSCore* pECS = ECSCore::GetInstance();

	// Load Match
	{
		const LambdaEngine::TArray<LambdaEngine::SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

		MatchDescription matchDescription =
		{
			.LevelHash = levelHashes[0]
		};

		Match::CreateMatch(&matchDescription);
	}

	// Robot
	{
		TArray<GUID_Lambda> animations;
		const uint32 robotGUID			= ResourceManager::LoadMeshFromFile("Robot/Rumba Dancing.fbx", animations);
		const uint32 robotAlbedoGUID	= ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_albedo.png", EFormat::FORMAT_R8G8B8A8_UNORM, true, true);
		const uint32 robotNormalGUID	= ResourceManager::LoadTextureFromFile("../Meshes/Robot/Textures/robot_normal.png", EFormat::FORMAT_R8G8B8A8_UNORM, true, true);

		TArray<GUID_Lambda> running		= ResourceManager::LoadAnimationsFromFile("Robot/Running.fbx");
		TArray<GUID_Lambda> walking		= ResourceManager::LoadAnimationsFromFile("Robot/Standard Walk.fbx");
		TArray<GUID_Lambda> thriller	= ResourceManager::LoadAnimationsFromFile("Robot/Thriller.fbx");
		TArray<GUID_Lambda> reload		= ResourceManager::LoadAnimationsFromFile("Robot/Reloading.fbx");

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
		robotAnimationComp.pGraph			= DBG_NEW AnimationGraph(DBG_NEW AnimationState("thriller", thriller[0]));
		robotAnimationComp.Pose.pSkeleton	= ResourceManager::GetMesh(robotGUID)->pSkeleton; // TODO: Safer way than getting the raw pointer (GUID for skeletons?)

		glm::vec3 position = glm::vec3(0.0f, 0.75f, -2.5f);
		glm::vec3 scale(1.0f);

		Entity entity = pECS->CreateEntity();
		m_Entities.PushBack(entity);
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "RobotUnwrappedTexture_0", 512, 512));
		pECS->AddComponent<PlayerBaseComponent>(entity, {});
		pECS->AddComponent<TeamComponent>(entity, { 1 });
		EntityMaskManager::AddExtensionToEntity(entity, PlayerBaseComponent::Type(), nullptr);

		position = glm::vec3(0.0f, 0.8f, 0.0f);
		robotAnimationComp.pGraph = DBG_NEW AnimationGraph(DBG_NEW AnimationState("walking", animations[0]));

		entity = pECS->CreateEntity();
		m_Entities.PushBack(entity);
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "RobotUnwrappedTexture_1", 512, 512));
		pECS->AddComponent<PlayerBaseComponent>(entity, {});
		pECS->AddComponent<TeamComponent>(entity, { 1 });
		EntityMaskManager::AddExtensionToEntity(entity, PlayerBaseComponent::Type(), nullptr);

		position = glm::vec3(-3.5f, 0.75f, 0.0f);
		robotAnimationComp.pGraph = DBG_NEW AnimationGraph(DBG_NEW AnimationState("running", running[0]));

		entity = pECS->CreateEntity();
		m_Entities.PushBack(entity);
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "RobotUnwrappedTexture_2", 512, 512));
		pECS->AddComponent<PlayerBaseComponent>(entity, {});
		pECS->AddComponent<TeamComponent>(entity, { 0 });
		EntityMaskManager::AddExtensionToEntity(entity, PlayerBaseComponent::Type(), nullptr);

		position = glm::vec3(3.5f, 0.75f, 0.0f);

		AnimationState* pReloadState = DBG_NEW AnimationState("reload");
		ClipNode* pReload = pReloadState->CreateClipNode(reload[0], 1.25, true);
		pReload->AddTrigger(ClipTrigger(0.9, [](const ClipNode& clip, AnimationGraph& graph)
			{
				graph.TransitionToState("running");
			}));

		ClipNode*	pRunning = pReloadState->CreateClipNode(running[0]);
		BlendNode*	pBlendNode = pReloadState->CreateBlendNode(pRunning, pReload, BlendInfo(0.8f, "mixamorig:Spine"));
		pReloadState->SetOutputNode(pBlendNode);

		AnimationGraph* pAnimationGraph = DBG_NEW AnimationGraph();
		pAnimationGraph->AddState(pReloadState);
		pAnimationGraph->AddState(DBG_NEW AnimationState("walking", walking[0]));
		pAnimationGraph->AddState(DBG_NEW AnimationState("running", running[0]));
		pAnimationGraph->AddTransition(DBG_NEW Transition("walking", "running", 0.1));
		pAnimationGraph->AddTransition(DBG_NEW Transition("running", "walking", 0.1));
		pAnimationGraph->AddTransition(DBG_NEW Transition("running", "reload", 0.1));
		pAnimationGraph->AddTransition(DBG_NEW Transition("reload", "running", 0.0));
		robotAnimationComp.pGraph = pAnimationGraph;

		entity = pECS->CreateEntity();
		m_Entities.PushBack(entity);
		pECS->AddComponent<PositionComponent>(entity, { true, position });
		pECS->AddComponent<ScaleComponent>(entity, { true, scale });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::identity<glm::quat>() });
		pECS->AddComponent<AnimationComponent>(entity, robotAnimationComp);
		pECS->AddComponent<MeshComponent>(entity, robotMeshComp);
		pECS->AddComponent<MeshPaintComponent>(entity, MeshPaint::CreateComponent(entity, "RobotUnwrappedTexture_3", 512, 512));
		pECS->AddComponent<PlayerBaseComponent>(entity, {});
		pECS->AddComponent<TeamComponent>(entity, { 0 });
		EntityMaskManager::AddExtensionToEntity(entity, PlayerBaseComponent::Type(), nullptr);

		// Audio
		GUID_Lambda soundGUID = ResourceManager::LoadSoundEffectFromFile("halo_theme.wav");
		ISoundInstance3D* pSoundInstance = DBG_NEW SoundInstance3DFMOD(AudioAPI::GetDevice());
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

	// Emitter
	{
		Entity entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(entity, { true, {-2.0f, 4.0f, 0.0f } });
		pECS->AddComponent<RotationComponent>(entity, { true, glm::rotate<float>(glm::identity<glm::quat>(), 0.f, g_DefaultUp) });
		pECS->AddComponent<ParticleEmitterComponent>(entity,
			ParticleEmitterComponent{
				.ParticleCount = 5,
				.EmitterShape = EEmitterShape::TUBE,
				.Velocity = 1.0f,
				.Acceleration = 0.0f,
				.BeginRadius = 0.5f,
				.TileIndex = 16,
				.AnimationCount = 4,
				.FirstAnimationIndex = 16,
				.Color = glm::vec4(0.7f, 0.5f, 0.3f, 1.f)
			}
		);
	}

	//Preload some resources
	{
		TArray<GUID_Lambda> animations;
		ResourceManager::LoadMeshFromFile("Robot/Standard Walk.fbx", animations);
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
	showTextureCMD.AddFlag("t", Arg::EType::STRING, 6);
	showTextureCMD.AddFlag("ps", Arg::EType::STRING);
	showTextureCMD.AddDescription("Show a texture resource which is used in the RenderGraph.\n\t'Example: debug_texture 1 -t TEXTURE_NAME1 TEXTURE_NAME2 ...'", { {"t", "The textures you want to display. (separated by spaces)"}, {"ps", "Which pixel shader you want to use."} });
	GameConsole::Get().BindCommand(showTextureCMD, [&, this](GameConsole::CallbackInput& input)->void
		{
			m_ShowTextureDebuggingWindow = input.Arguments.GetFront().Value.Boolean;

			auto textureNameIt = input.Flags.find("t");
			auto shaderNameIt = input.Flags.find("ps");

			GUID_Lambda textureDebuggingShaderGUID = shaderNameIt != input.Flags.end() ? ResourceManager::GetShaderGUID(shaderNameIt->second.Arg.Value.String) : GUID_NONE;
			if (textureNameIt != input.Flags.end())
			{
				m_TextureDebuggingNames.Resize(textureNameIt->second.NumUsedArgs);
				for (uint32 i = 0; i < textureNameIt->second.NumUsedArgs; i++)
				{
					m_TextureDebuggingNames[i].ResourceName = textureNameIt->second.Args[i].Value.String;
					m_TextureDebuggingNames[i].PixelShaderGUID = textureDebuggingShaderGUID;
				}
			}
		});

	SingleplayerInitializer::InitSingleplayer();
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

	m_MultiplayerClient.TickMainThreadInternal(delta);

	if constexpr (IMGUI_ENABLED)
	{
		RenderImgui();
	}

	// Debugging Emitters
	ECSCore* ecsCore = ECSCore::GetInstance();
	static uint32 indexEmitters = 0;
	static LambdaEngine::Timestamp time;
	const uint32 emitterCount = 10U;
	if (m_DebugEmitters)
	{
		time += delta;
		if (time.AsSeconds() > 0.1f)
		{
			uint32 modIndex = indexEmitters % emitterCount;

			if (indexEmitters < emitterCount)
			{
				Entity e = ecsCore->CreateEntity();
				m_Emitters[modIndex] = e;

				ecsCore->AddComponent<PositionComponent>(e, { true, {0.0f, 2.0f + Random::Float32(-1.0f, 1.0f), -4.f + float(modIndex) } });
				ecsCore->AddComponent<RotationComponent>(e, { true,	GetRotationQuaternion(glm::normalize(glm::vec3(float(modIndex % 2U), float(modIndex % 3U), float(modIndex % 5U)))) });
				ecsCore->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{
					.OneTime = true,
					.Explosive = 0.9f,
					.SpawnDelay = 0.01f,
					.ParticleCount = 256,
					.Velocity = 1.0f + Random::Float32(-3.f, 3.f),
					.Acceleration = 0.0f,
					.Gravity = Random::Float32(-5.0f, 5.0f),
					.LifeTime = Random::Float32(1.0f, 3.0f),
					.BeginRadius = 0.1f + Random::Float32(0.0f, 0.5f),
					.Color = glm::vec4(modIndex % 2U, modIndex % 3U, modIndex % 5U, 1.0f),
					});
			}
			else
			{
				auto& emitterComp = ecsCore->GetComponent<ParticleEmitterComponent>(m_Emitters[modIndex]);

				emitterComp.Explosive = 0.9f + Random::Float32(0.f, 0.1f);
				emitterComp.BeginRadius = 0.1f + Random::Float32(0.0f, 0.5f);
				emitterComp.Velocity = 1.0f + Random::Float32(-3.f, 3.f);
				emitterComp.Gravity = Random::Float32(-5.0f, 5.0f);
				emitterComp.LifeTime = Random::Float32(1.0f, 3.0f);
				emitterComp.Color = glm::vec4(indexEmitters % 2U, indexEmitters % 3U, indexEmitters % 5U, 1.0f),
				emitterComp.Active = true;
			}

			indexEmitters++;
			time = 0;
		}
	}
}

void SandboxState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_MultiplayerClient.FixedTickMainThreadInternal(delta);
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
				for (ImGuiTexture& imGuiTexture : m_TextureDebuggingNames)
				{
					ImGui::Image(&imGuiTexture, ImGui::GetWindowSize());
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

	if (event.Key == EKey::KEY_DELETE)
	{
		if (!m_Entities.IsEmpty())
		{
			const uint32 numEntities = m_Entities.GetSize();
			const uint32 index = Random::UInt32(0, numEntities-1);
			Entity entity = m_Entities[index];
			m_Entities.Erase(m_Entities.Begin() + index);
			ECSCore::GetInstance()->RemoveEntity(entity);

			std::string info = "Removed entity with index [" + std::to_string(index) + "/" + std::to_string(numEntities) + "]!";
			GameConsole::Get().PushInfo(info);
			LOG_INFO(info.c_str());
		}
	}

	// Debugging Lights
	static uint32 indexLights = 0;
	static bool removeLights = true;
	ECSCore* ecsCore = ECSCore::GetInstance();
	if (event.Key == EKey::KEY_9)
	{
		uint32 modIndex = indexLights % 10U;
		if (modIndex == 0U)
			removeLights = !removeLights;

		if (!removeLights)
		{
			Entity e = ecsCore->CreateEntity();
			m_PointLights[modIndex] = e;

			ecsCore->AddComponent<PositionComponent>(e, { true, {0.0f, 2.0f, -5.0f + modIndex} });
			ecsCore->AddComponent<PointLightComponent>(e, PointLightComponent{ .ColorIntensity = {modIndex % 2U, modIndex % 3U, modIndex % 5U, 25.0f} });
		}
		else
		{
			ecsCore->RemoveEntity(m_PointLights[modIndex]);
		}

		indexLights++;
	}

	// Debugging Emitters
	if (event.Key == EKey::KEY_8)
	{
		m_DebugEmitters = !m_DebugEmitters;
	}

	return true;
}
