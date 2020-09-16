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

#include "Math/Random.h"
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
	TESTING,
	CUBEMAP
};

Sandbox::Sandbox()
	: Game()
{
	using namespace LambdaEngine;

	m_RenderGraphWindow = EngineConfig::GetBoolProperty("ShowRenderGraph");
	m_ShowDemoWindow = EngineConfig::GetBoolProperty("ShowDemo");
	m_DebuggingWindow = EngineConfig::GetBoolProperty("Debugging");

	EventQueue::RegisterEventHandler<KeyPressedEvent>(EventHandler(this, &Sandbox::OnKeyPressed));

	ShaderReflection shaderReflection;
	ResourceLoader::CreateShaderReflection("../Assets/Shaders/Raygen.rgen", FShaderStageFlag::SHADER_STAGE_FLAG_RAYGEN_SHADER, EShaderLang::SHADER_LANG_GLSL, &shaderReflection);

	m_pScene = DBG_NEW Scene();

	GraphicsDeviceFeatureDesc deviceFeatures = {};
	RenderSystem::GetDevice()->QueryDeviceFeatures(&deviceFeatures);

	SceneDesc sceneDesc = { };
	sceneDesc.Name				= "Test Scene";
	sceneDesc.RayTracingEnabled = deviceFeatures.RayTracing && EngineConfig::GetBoolProperty("RayTracingEnabled");
	m_pScene->Init(sceneDesc);

	EScene scene = EScene::TESTING;

	//m_pScene->SetDirectionalLight(directionalLight);

	AreaLightObject areaLight;
	areaLight.Type = EAreaLightType::QUAD;
	areaLight.Material = GUID_MATERIAL_DEFAULT_EMISSIVE;

	if (scene == EScene::SPONZA)
	{
		//Lights
		{
			//glm::vec3 position(0.0f, 6.0f, 0.0f);
			//glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			//glm::vec3 scale(1.5f);

			//glm::mat4 transform(1.0f);
			//transform = glm::translate(transform, position);
			//transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			//transform = glm::scale(transform, scale);

			//InstanceIndexAndTransform instanceIndexAndTransform;
			//instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			//instanceIndexAndTransform.Position		= position;
			//instanceIndexAndTransform.Rotation		= rotation;
			//instanceIndexAndTransform.Scale			= scale;

			//m_LightInstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
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

			for (uint32 i = 0; i < sceneGameObjects.GetSize(); i++)
			{
				m_pScene->AddGameObject(i, sceneGameObjects[i], transform, true, false);

				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = i;
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
			//glm::vec3 position(0.0f, 1.95f, 0.0f);
			//glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			//glm::vec3 scale(0.2f);

			//glm::mat4 transform(1.0f);
			//transform = glm::translate(transform, position);
			//transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			//transform = glm::scale(transform, scale);

			//InstanceIndexAndTransform instanceIndexAndTransform;
			//instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			//instanceIndexAndTransform.Position		= position;
			//instanceIndexAndTransform.Rotation		= rotation;
			//instanceIndexAndTransform.Scale			= scale;

			//m_LightInstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
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

			for (uint32 i = 0; i < sceneGameObjects.GetSize(); i++)
			{
				m_pScene->AddGameObject(i, sceneGameObjects[i], transform, true, false);

				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = i;
				instanceIndexAndTransform.Position = position;
				instanceIndexAndTransform.Rotation = rotation;
				instanceIndexAndTransform.Scale = scale;

				m_InstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
			}
		}
	}
	else if (scene == EScene::TESTING)
	{
		uint32 entityID = 0;

		//Lights
		{
			//glm::vec3 position(0.0f, 6.0f, 0.0f);
			//glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
			//glm::vec3 scale(1.5f);

			//glm::mat4 transform(1.0f);
			//transform = glm::translate(transform, position);
			//transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			//transform = glm::scale(transform, scale);

			//InstanceIndexAndTransform instanceIndexAndTransform;
			//instanceIndexAndTransform.InstanceIndex = m_pScene->AddAreaLight(areaLight, transform);
			//instanceIndexAndTransform.Position		= position;
			//instanceIndexAndTransform.Rotation		= rotation;
			//instanceIndexAndTransform.Scale			= scale;

			//m_LightInstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
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

			for (uint32 i = 0; i < sceneGameObjects.GetSize(); i++)
			{
				m_pScene->AddGameObject(entityID, sceneGameObjects[i], transform, true, false);

				InstanceIndexAndTransform instanceIndexAndTransform;
				instanceIndexAndTransform.InstanceIndex = entityID;
				instanceIndexAndTransform.Position = position;
				instanceIndexAndTransform.Rotation = rotation;
				instanceIndexAndTransform.Scale = scale;

				m_InstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
				entityID++;
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

					m_pScene->AddGameObject(entityID, sphereGameObject, transform, true, false);

					InstanceIndexAndTransform instanceIndexAndTransform;
					instanceIndexAndTransform.InstanceIndex = entityID;
					instanceIndexAndTransform.Position		= position;
					instanceIndexAndTransform.Rotation		= glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
					instanceIndexAndTransform.Scale			= scale;

					m_InstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
					entityID++;
				}
			}
		}
	}
	else if (scene == EScene::CUBEMAP)
	{
		//Cube
		{
			TArray<GameObject> sceneGameObjects;
			uint32 cubeMeshGUID = ResourceManager::LoadMeshFromFile("cube.obj");

			glm::vec3 position(0.0f, 0.0f, 0.0f);
			glm::vec4 rotation(0.0f, 1.0f, 0.0f, 0.0f);
			glm::vec3 scale(1.0f);

			glm::mat4 transform(1.0f);
			transform = glm::translate(transform, position);
			transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
			transform = glm::scale(transform, scale);

			MaterialProperties materialProperties;
			materialProperties.Albedo = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
			materialProperties.Roughness = 0.1f;
			materialProperties.Metallic = 0.1f;

			GameObject sphereGameObject = {};
			sphereGameObject.Mesh = cubeMeshGUID;
			sphereGameObject.Material = ResourceManager::LoadMaterialFromMemory(
				"Default r: " + std::to_string(0.1f) + " m: " + std::to_string(0.1f),
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_NORMAL_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				GUID_TEXTURE_DEFAULT_COLOR_MAP,
				materialProperties);


			m_pScene->AddGameObject(0, sphereGameObject, transform, true, false);

			InstanceIndexAndTransform instanceIndexAndTransform;
			instanceIndexAndTransform.InstanceIndex = 0;
			instanceIndexAndTransform.Position = position;
			instanceIndexAndTransform.Rotation = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			instanceIndexAndTransform.Scale = scale;

			m_InstanceIndicesAndTransforms.PushBack(instanceIndexAndTransform);
		}
	}


	m_pScene->Finalize();
	Renderer::SetScene(m_pScene);

	m_pCamera = DBG_NEW Camera();

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= EngineConfig::GetFloatProperty("CameraFOV");
	cameraDesc.Width		= window->GetWidth();
	cameraDesc.Height		= window->GetHeight();
	cameraDesc.NearPlane	= EngineConfig::GetFloatProperty("CameraNearPlane");
	cameraDesc.FarPlane		= EngineConfig::GetFloatProperty("CameraFarPlane");

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
	cmd3.Init("show_debug_window", true);
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

	return;
}

Sandbox::~Sandbox()
{
	using namespace LambdaEngine;

	EventQueue::UnregisterEventHandler<KeyPressedEvent>(EventHandler(this, &Sandbox::OnKeyPressed));

	SAFEDELETE(m_pScene);
	SAFEDELETE(m_pCamera);

	SAFEDELETE(m_pRenderGraphEditor);

	SAFEDELETE(m_pPointLightsBuffer);
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
	else if (event.Key == EKey::KEY_KEYPAD_1)
	{
		Renderer::GetRenderGraph()->TriggerRenderStage("POINT_LIGHT_SHADOWMAPS");
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

	//float32 test = Random::Float32();

	//PushConstantsUpdate pushConstantsUpdateTest = {};
	//pushConstantsUpdateTest.RenderStageName		= "DEMO";
	//pushConstantsUpdateTest.pData				= &test;
	//pushConstantsUpdateTest.DataSize			= sizeof(float32);
	//Renderer::GetRenderGraph()->UpdatePushConstants(&pushConstantsUpdateTest);

	Renderer::Render();
}

void Sandbox::OnRenderGraphRecreate(LambdaEngine::RenderGraph* pRenderGraph)
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

	ResourceUpdateDesc pointLightsBuffer = {};
	pointLightsBuffer.ResourceName						= "POINT_LIGHTS_BUFFER";
	pointLightsBuffer.ExternalBufferUpdate.ppBuffer		= &m_pPointLightsBuffer;

	pRenderGraph->UpdateResource(&pointLightsBuffer);
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

		Renderer::GetRenderGraph()->UpdateResource(&blueNoiseUpdateDesc);
	}

	// For Skybox RenderGraph
	{
		String skybox[]
		{
			"Skybox/right.png",
			"Skybox/left.png",
			"Skybox/top.png",
			"Skybox/bottom.png",
			"Skybox/front.png",
			"Skybox/back.png"
		};

		GUID_Lambda cubemapTexID = ResourceManager::LoadCubeTexturesArrayFromFile("Cubemap Texture", skybox, 1, EFormat::FORMAT_R8G8B8A8_UNORM, false);

		Texture* pCubeTexture			= ResourceManager::GetTexture(cubemapTexID);
		TextureView* pCubeTextureView	= ResourceManager::GetTextureView(cubemapTexID);
		Sampler* pNearestSampler		= Sampler::GetNearestSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName = "SKYBOX";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures		= &pCubeTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews	= &pCubeTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pNearestSampler;

		Renderer::GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	//Point Lights Test
	{
		float pointLightNearPlane	= 1.0f;
		float pointLightFarPlane	= 25.0f;

		glm::mat4 pointLightProj = glm::perspective(glm::radians(90.0f), 1.0f, pointLightNearPlane, pointLightFarPlane);

		glm::vec3 pointLightPosition0 = glm::vec3(1.0f, 2.0f, 0.0f);
		glm::vec3 pointLightPosition1 = glm::vec3(-1.0f, 2.0f, 0.0f);

		PointLight pointLightsBuffer[2];
		pointLightsBuffer[0].Position = glm::vec4(pointLightPosition0, 1.0f);
		pointLightsBuffer[1].Position = glm::vec4(pointLightPosition1, 1.0f);

		pointLightsBuffer[0].Transforms[0]		= pointLightProj * glm::lookAt(pointLightPosition0, pointLightPosition0 + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		pointLightsBuffer[0].Transforms[1]		= pointLightProj * glm::lookAt(pointLightPosition0, pointLightPosition0 + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		pointLightsBuffer[0].Transforms[2]		= pointLightProj * glm::lookAt(pointLightPosition0, pointLightPosition0 + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f));
		pointLightsBuffer[0].Transforms[3]		= pointLightProj * glm::lookAt(pointLightPosition0, pointLightPosition0 + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
		pointLightsBuffer[0].Transforms[4]		= pointLightProj * glm::lookAt(pointLightPosition0, pointLightPosition0 + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		pointLightsBuffer[0].Transforms[5]		= pointLightProj * glm::lookAt(pointLightPosition0, pointLightPosition0 + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		
		pointLightsBuffer[1].Transforms[0]		= pointLightProj * glm::lookAt(pointLightPosition1, pointLightPosition1 + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		pointLightsBuffer[1].Transforms[1]		= pointLightProj * glm::lookAt(pointLightPosition1, pointLightPosition1 + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		pointLightsBuffer[1].Transforms[2]		= pointLightProj * glm::lookAt(pointLightPosition1, pointLightPosition1 + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f));
		pointLightsBuffer[1].Transforms[3]		= pointLightProj * glm::lookAt(pointLightPosition1, pointLightPosition1 + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
		pointLightsBuffer[1].Transforms[4]		= pointLightProj * glm::lookAt(pointLightPosition1, pointLightPosition1 + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		pointLightsBuffer[1].Transforms[5]		= pointLightProj * glm::lookAt(pointLightPosition1, pointLightPosition1 + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f));

		BufferDesc bufferDesc = {};
		bufferDesc.DebugName		= "POINT_LIGHTS_BUFFER";
		bufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		bufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
		bufferDesc.SizeInBytes		= sizeof(pointLightsBuffer);

		m_pPointLightsBuffer = RenderSystem::GetDevice()->CreateBuffer(&bufferDesc);

		void* pMapped = m_pPointLightsBuffer->Map();
		memcpy(pMapped, &pointLightsBuffer, sizeof(pointLightsBuffer));
		m_pPointLightsBuffer->Unmap();

		ResourceUpdateDesc pointLightsBufferUpdate = {};
		pointLightsBufferUpdate.ResourceName						= "POINT_LIGHTS_BUFFER";
		pointLightsBufferUpdate.ExternalBufferUpdate.ppBuffer		= &m_pPointLightsBuffer;

		Renderer::GetRenderGraph()->UpdateResource(&pointLightsBufferUpdate);

		float pointLightPushConstantData[2];
		pointLightPushConstantData[0]		= pointLightNearPlane;
		pointLightPushConstantData[1]		= pointLightFarPlane;

		PushConstantsUpdate pushConstantUpdate = {};
		pushConstantUpdate.pData			= &pointLightPushConstantData;
		pushConstantUpdate.DataSize			= sizeof(pointLightPushConstantData);

		pushConstantUpdate.RenderStageName	= "POINT_LIGHT_SHADOWMAPS";
		Renderer::GetRenderGraph()->UpdatePushConstants(&pushConstantUpdate);

		pushConstantUpdate.RenderStageName	= "DEMO";
		Renderer::GetRenderGraph()->UpdatePushConstants(&pushConstantUpdate);
	}
	Renderer::GetRenderGraph()->AddCreateHandler(this);

	return true;
}