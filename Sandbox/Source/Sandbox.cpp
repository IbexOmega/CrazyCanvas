#include "Sandbox.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Containers/TSharedPtr.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/ImGuiRenderer.h"
#include "Rendering/Renderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraphEditor.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/CommandQueue.h"

#include "Audio/AudioSystem.h"
#include "Audio/API/ISoundEffect3D.h"
#include "Audio/API/ISoundInstance3D.h"
#include "Audio/API/IAudioGeometry.h"
#include "Audio/API/IReverbSphere.h"
#include "Audio/API/IMusic.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Application/API/Events/EventQueue.h"

#include "Engine/EngineConfig.h"

#include "Game/Scene.h"
#include "Game/GameConsole.h"

#include "Time/API/Clock.h"

#include "Threading/API/Thread.h"

#include "Debug/Profiler.h"

#include <imgui.h>

constexpr const float DEFAULT_DIR_LIGHT_R			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_G			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_B			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_STRENGTH	= 0.0f;

constexpr const uint32 NUM_BLUE_NOISE_LUTS = 128;

//#hej Herman vll du bli tillsammans / Kim Kardashian ;)

enum class EScene
{
	SPONZA,
	CORNELL,
	TESTING
};

Sandbox::Sandbox()
	: Game()
{
	using namespace LambdaEngine;

	m_RenderGraphWindow = false;
	m_ShowDemoWindow = false;
	m_DebuggingWindow = false;

	EventQueue::RegisterEventHandler<KeyPressedEvent>(EventHandler(this, &Sandbox::OnKeyPressed));

	ShaderReflection shaderReflection;
	ResourceLoader::CreateShaderReflection("../Assets/Shaders/Raygen.rgen", FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER, EShaderLang::SHADER_LANG_GLSL, &shaderReflection);

	m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice());

	GraphicsDeviceFeatureDesc deviceFeatures = {};
	RenderSystem::GetDevice()->QueryDeviceFeatures(&deviceFeatures);

	SceneDesc sceneDesc = { };
	sceneDesc.Name				= "Test Scene";
	sceneDesc.RayTracingEnabled = deviceFeatures.RayTracing && EngineConfig::GetBoolProperty("RayTracingEnabled");
	m_pScene->Init(sceneDesc);

	m_DirectionalLightAngle	= glm::half_pi<float>();
	m_DirectionalLightStrength[0] = DEFAULT_DIR_LIGHT_R;
	m_DirectionalLightStrength[1] = DEFAULT_DIR_LIGHT_G;
	m_DirectionalLightStrength[2] = DEFAULT_DIR_LIGHT_B;
	m_DirectionalLightStrength[3] = DEFAULT_DIR_LIGHT_STRENGTH;

	DirectionalLight directionalLight;
	directionalLight.Direction			= glm::vec4(glm::normalize(glm::vec3(glm::cos(m_DirectionalLightAngle), glm::sin(m_DirectionalLightAngle), 0.0f)), 0.0f);
	directionalLight.EmittedRadiance	= glm::vec4(glm::vec3(m_DirectionalLightStrength[0], m_DirectionalLightStrength[1], m_DirectionalLightStrength[2]) * m_DirectionalLightStrength[3], 0.0f);

	EScene scene = EScene::TESTING;

	m_pScene->SetDirectionalLight(directionalLight);

	AreaLightObject areaLight;
	areaLight.Type = EAreaLightType::QUAD;
	areaLight.Material = GUID_MATERIAL_DEFAULT_EMISSIVE;

	if (scene == EScene::SPONZA)
	{
		//Lights
		{
			glm::vec3 position(0.0f, 6.0f, 0.0f);
			glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			glm::vec3 scale(1.5f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			InstanceIndexAndTransform instanceIndexAndTransform;
			instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			instanceIndexAndTransform.Position		= position;
			instanceIndexAndTransform.Rotation		= rotation;
			instanceIndexAndTransform.Scale			= scale;

			m_LightInstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
		}

		//Scene
		{
			TArray<GameObject>	sceneGameObjects;
			ResourceManager::LoadSceneFromFile("sponza/sponza.obj", sceneGameObjects);

			glm::vec3 position(0.0f, 0.0f, 0.0f);
			glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec3 scale(0.01f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			for (GameObject& gameObject : sceneGameObjects)
			{
				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = m_pScene->AddDynamicGameObject(gameObject, transform);
				instanceIndexAndTransform.Position = position;
				instanceIndexAndTransform.Rotation = rotation;
				instanceIndexAndTransform.Scale = scale;

				m_InstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
			}
		}
	}
	else if (scene == EScene::CORNELL)
	{
		//Lights
		{
			glm::vec3 position(0.0f, 1.95f, 0.0f);
			glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			glm::vec3 scale(0.2f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			InstanceIndexAndTransform instanceIndexAndTransform;
			instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			instanceIndexAndTransform.Position		= position;
			instanceIndexAndTransform.Rotation		= rotation;
			instanceIndexAndTransform.Scale			= scale;

			m_LightInstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
		}

		//Scene
		{
			TArray<GameObject>	sceneGameObjects;
			ResourceManager::LoadSceneFromFile("CornellBox/CornellBox-Original-No-Light.obj", sceneGameObjects);

			glm::vec3 position(0.0f, 0.0f, 0.0f);
			glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec3 scale(1.0f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			for (GameObject& gameObject : sceneGameObjects)
			{
				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = m_pScene->AddDynamicGameObject(gameObject, transform);
				instanceIndexAndTransform.Position = position;
				instanceIndexAndTransform.Rotation = rotation;
				instanceIndexAndTransform.Scale = scale;

				m_InstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
			}
		}
	}
	else if (scene == EScene::TESTING)
	{
		//Lights
		{
			glm::vec3 position(0.0f, 6.0f, 0.0f);
			glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			glm::vec3 scale(1.5f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			InstanceIndexAndTransform instanceIndexAndTransform;
			instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			instanceIndexAndTransform.Position		= position;
			instanceIndexAndTransform.Rotation		= rotation;
			instanceIndexAndTransform.Scale			= scale;

			m_LightInstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
		}

		//Scene
		{
			TArray<GameObject> sceneGameObjects;
			ResourceManager::LoadSceneFromFile("Testing/Testing.obj", sceneGameObjects);

			glm::vec3 position(0.0f, 0.0f, 0.0f);
			glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec3 scale(1.0f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			for (GameObject& gameObject : sceneGameObjects)
			{
				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = m_pScene->AddDynamicGameObject(gameObject, transform);
				instanceIndexAndTransform.Position = position;
				instanceIndexAndTransform.Rotation = rotation;
				instanceIndexAndTransform.Scale = scale;

				m_InstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
			}
		}

		//Sphere Grid
		{
			uint32 sphereMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

			uint32 gridRadius = 5;

			for (uint32 y = 0; y < gridRadius; y++)
			{
				float32 roughness = y / float32(gridRadius - 1);

				for (uint32 x = 0; x < gridRadius; x++)
				{
					float32 metallic = x / float32(gridRadius - 1);

					MaterialProperties materialProperties;
					materialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
					materialProperties.Roughness	= roughness;
					materialProperties.Metallic		= metallic;

					GameObject sphereGameObject = {};
					sphereGameObject.Mesh		= sphereMeshGUID;
					sphereGameObject.Material	= ResourceManager::LoadMaterialFromMemory(
						"Default r: " + std::to_string(roughness) + " m: " + std::to_string(metallic),
						GUID_TEXTURE_DEFAULT_COLOR_MAP,
						GUID_TEXTURE_DEFAULT_NORMAL_MAP,
						GUID_TEXTURE_DEFAULT_COLOR_MAP,
						GUID_TEXTURE_DEFAULT_COLOR_MAP,
						GUID_TEXTURE_DEFAULT_COLOR_MAP,
						materialProperties);

					glm::vec3 position(-float32(gridRadius) * 0.5f + x, 1.0f + y, 5.0f);
					glm::vec3 scale(1.0f);

					glm::mat4 transform(1.0f);
					transform = glm::translate(transform, position);
					transform = glm::scale(transform, scale);

					InstanceIndexAndTransform instanceIndexAndTransform;
					instanceIndexAndTransform.InstanceIndex = m_pScene->AddDynamicGameObject(sphereGameObject, transform);
					instanceIndexAndTransform.Position		= position;
					instanceIndexAndTransform.Rotation		= glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
					instanceIndexAndTransform.Scale			= scale;

					m_InstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
				}
			}
		}
	}

	m_pScene->Finalize();
	Renderer::SetScene(m_pScene);

	m_pCamera = DBG_NEW Camera();

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= 90.0f;
	cameraDesc.Width		= window->GetWidth();
	cameraDesc.Height		= window->GetHeight();
	cameraDesc.NearPlane	= 0.001f;
	cameraDesc.FarPlane		= 1000.0f;

	m_pCamera->Init(cameraDesc);

	LoadRendererResources();

	m_pScene->UpdateCamera(m_pCamera);

	if (IMGUI_ENABLED)
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
		m_RenderGraphWindow = input.Arguments.GetFront().Value.B;
		});

	ConsoleCommand cmd2;
	cmd2.Init("imgui_demo", true);
	cmd2.AddArg(Arg::EType::BOOL);
	cmd2.AddDescription("Activate/Deactivate demo window.\n\t'imgui_demo true'");
	GameConsole::Get().BindCommand(cmd2, [&, this](GameConsole::CallbackInput& input)->void {
		m_ShowDemoWindow = input.Arguments.GetFront().Value.B;
		});

	ConsoleCommand cmd3;
	cmd3.Init("show_debug_window", true);
	cmd3.AddArg(Arg::EType::BOOL);
	cmd3.AddDescription("Activate/Deactivate debugging window.\n\t'show_debug_window true'");
	GameConsole::Get().BindCommand(cmd3, [&, this](GameConsole::CallbackInput& input)->void {
		m_DebuggingWindow = input.Arguments.GetFront().Value.B;
		});

	return;
}

Sandbox::~Sandbox()
{
	using namespace LambdaEngine;

	EventQueue::UnregisterEventHandler<KeyPressedEvent>(EventHandler(this, &Sandbox::OnKeyPressed));

	SAFEDELETE(m_pScene);
	SAFEDELETE(m_pCamera);

	SAFEDELETE(m_pRenderGraphEditor);
}

bool Sandbox::OnKeyPressed(const LambdaEngine::KeyPressedEvent& event)
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

	if (event.Key == EKey::KEY_KEYPAD_5)
	{
		RenderSystem::GetGraphicsQueue()->Flush();
		RenderSystem::GetComputeQueue()->Flush();
		ResourceManager::ReloadAllShaders();
		PipelineStateManager::ReloadPipelineStates();
	}

	return true;
}

void Sandbox::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	m_pRenderGraphEditor->Update();
	Render(delta);
}

void Sandbox::FixedTick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	m_pCamera->HandleInput(delta);
	m_pCamera->Update();
	m_pScene->UpdateCamera(m_pCamera);
}

void Sandbox::Render(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
	float32 renderWidth = (float32)mainWindow->GetWidth();
	float32 renderHeight = (float32)mainWindow->GetHeight();
	float32 renderAspectRatio = renderWidth / renderHeight;

	if (IMGUI_ENABLED)
	{
		ImGuiRenderer::Get().DrawUI([&]()
		{
			if (m_RenderGraphWindow)
				m_pRenderGraphEditor->RenderGUI();

			if (m_ShowDemoWindow)
				ImGui::ShowDemoWindow();

			if (m_DebuggingWindow)
			{
				Profiler::Render(delta);
			}
			
		});
	}

	Renderer::Render();
}

void Sandbox::OnRenderGraphRecreate(LambdaEngine::RenderGraph* pRenderGraph)
{
	using namespace LambdaEngine;

	GUID_Lambda blueNoiseID = ResourceManager::GetTextureGUID("Blue Noise Texture");

	Texture* pBlueNoiseTexture				= ResourceManager::GetTexture(blueNoiseID);
	TextureView* pBlueNoiseTextureView		= ResourceManager::GetTextureView(blueNoiseID);
	Sampler* pNearestSampler				= Sampler::GetNearestSampler();

	ResourceUpdateDesc blueNoiseUpdateDesc = {};
	blueNoiseUpdateDesc.ResourceName								= "BLUE_NOISE_LUT";
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures			= &pBlueNoiseTexture;
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews		= &pBlueNoiseTextureView;
	blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers			= &pNearestSampler;

	Renderer::GetRenderGraph()->UpdateResource(blueNoiseUpdateDesc);
}

namespace LambdaEngine
{
	Game* CreateGame()
	{
		Sandbox* pSandbox = DBG_NEW Sandbox();
		return pSandbox;
	}
}

bool Sandbox::LoadRendererResources()
{
	using namespace LambdaEngine;

	{
		String blueNoiseLUTFileNames[NUM_BLUE_NOISE_LUTS];

		for (uint32 i = 0; i < NUM_BLUE_NOISE_LUTS; i++)
		{
			char str[5];
			snprintf(str, 5, "%04d", i);
			blueNoiseLUTFileNames[i] = "LUTs/BlueNoise/256_256/HDR_RGBA_" + std::string(str) + ".png";
		}

		GUID_Lambda blueNoiseID = ResourceManager::LoadTextureArrayFromFile("Blue Noise Texture", blueNoiseLUTFileNames, NUM_BLUE_NOISE_LUTS, EFormat::FORMAT_R16_UNORM, false);

		Texture* pBlueNoiseTexture				= ResourceManager::GetTexture(blueNoiseID);
		TextureView* pBlueNoiseTextureView		= ResourceManager::GetTextureView(blueNoiseID);
		Sampler* pNearestSampler				= Sampler::GetNearestSampler();

		ResourceUpdateDesc blueNoiseUpdateDesc = {};
		blueNoiseUpdateDesc.ResourceName								= "BLUE_NOISE_LUT";
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures			= &pBlueNoiseTexture;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews		= &pBlueNoiseTextureView;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers			= &pNearestSampler;

		Renderer::GetRenderGraph()->UpdateResource(blueNoiseUpdateDesc);
	}

	Renderer::GetRenderGraph()->AddCreateHandler(this);

	return true;
}