#include "Sandbox.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Containers/TSharedPtr.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/ImGuiRenderer.h"
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

#include "Game/GameConsole.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "States/SandboxState.h"

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

	GraphicsDeviceFeatureDesc deviceFeatures = {};
	RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);

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

	StateManager::GetInstance()->EnqueueStateTransition(DBG_NEW(SandboxState), STATE_TRANSITION::PUSH);

	RenderSystem::GetInstance().SetCamera(m_pCamera);

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
		RenderAPI::GetGraphicsQueue()->Flush();
		RenderAPI::GetComputeQueue()->Flush();
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

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&blueNoiseUpdateDesc);
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

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
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

		m_pPointLightsBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);

		void* pMapped = m_pPointLightsBuffer->Map();
		memcpy(pMapped, &pointLightsBuffer, sizeof(pointLightsBuffer));
		m_pPointLightsBuffer->Unmap();

		ResourceUpdateDesc pointLightsBufferUpdate = {};
		pointLightsBufferUpdate.ResourceName						= "POINT_LIGHTS_BUFFER";
		pointLightsBufferUpdate.ExternalBufferUpdate.ppBuffer		= &m_pPointLightsBuffer;


		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&pointLightsBufferUpdate);

		float pointLightPushConstantData[2];
		pointLightPushConstantData[0]		= pointLightNearPlane;
		pointLightPushConstantData[1]		= pointLightFarPlane;

		PushConstantsUpdate pushConstantUpdate = {};
		pushConstantUpdate.pData			= &pointLightPushConstantData;
		pushConstantUpdate.DataSize			= sizeof(pointLightPushConstantData);

		pushConstantUpdate.RenderStageName	= "POINT_LIGHT_SHADOWMAPS";
		
		RenderSystem::GetInstance().GetRenderGraph()->UpdatePushConstants(&pushConstantUpdate);

		pushConstantUpdate.RenderStageName	= "DEMO";

		RenderSystem::GetInstance().GetRenderGraph()->UpdatePushConstants(&pushConstantUpdate);
	}

	RenderSystem::GetInstance().GetRenderGraph()->AddCreateHandler(this);

	return true;
}