#include "Sandbox.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Containers/TSharedPtr.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraphEditor.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/CommandQueue.h"

#include "Audio/AudioAPI.h"
#include "Audio/API/ISoundEffect3D.h"
#include "Audio/API/ISoundInstance3D.h"
#include "Audio/API/IAudioGeometry.h"
#include "Audio/API/IReverbSphere.h"
#include "Audio/API/IMusic.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/DebugEvents.h"

#include "Engine/EngineConfig.h"

#include "Game/GameConsole.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/StateManager.h"
#include "States/SandboxState.h"

#include "Time/API/Clock.h"

#include "Threading/API/Thread.h"

#include "Math/Random.h"
#include "Debug/Profiler.h"

#include <argh/argh.h>
#include <imgui.h>

#include "Containers/TSharedPtr.h"

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

	LoadRendererResources();

	StateManager::GetInstance()->EnqueueStateTransition(DBG_NEW(SandboxState), STATE_TRANSITION::PUSH);

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
	showTextureCMD.AddFlag("t", Arg::EType::STRING, 6);
	showTextureCMD.AddFlag("ps", Arg::EType::STRING);
	showTextureCMD.AddDescription("Show a texture resource which is used in the RenderGraph.\n\t'Example: debug_texture 1 -t TEXTURE_NAME1 TEXTURE_NAME2 ...'", { {"t", "The textures you want to display. (separated by spaces)"}, {"ps", "Which pixel shader you want to use."} });
	GameConsole::Get().BindCommand(showTextureCMD, [&, this](GameConsole::CallbackInput& input)->void
		{
			m_ShowTextureDebuggingWindow = input.Arguments.GetFront().Value.Boolean;

			auto textureNameIt				= input.Flags.find("t");
			auto shaderNameIt				= input.Flags.find("ps");

			GUID_Lambda textureDebuggingShaderGUID	= shaderNameIt != input.Flags.end() ? ResourceManager::GetShaderGUID(shaderNameIt->second.Arg.Value.String) : GUID_NONE;
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

	return;
}

Sandbox::~Sandbox()
{
	using namespace LambdaEngine;

	EventQueue::UnregisterEventHandler<KeyPressedEvent>(EventHandler(this, &Sandbox::OnKeyPressed));

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

	if (event.Key == EKey::KEY_5)
	{
		EventQueue::SendEvent(ShaderRecompileEvent());
		EventQueue::SendEvent(PipelineStateRecompileEvent());
	}

	return true;
}

void Sandbox::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	m_pRenderGraphEditor->Update();
	Profiler::Tick(delta);
	Render(delta);
}

void Sandbox::FixedTick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	using namespace LambdaEngine;
}

void Sandbox::Render(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
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
				Profiler::Render();
			}

			if (m_ShowTextureDebuggingWindow)
			{
				if (ImGui::Begin("Texture Debugging"))
				{
					if (!m_TextureDebuggingNames.IsEmpty())
					{
						for (ImGuiTexture& imGuiTexture : m_TextureDebuggingNames)
						{
							ImGui::Image(&imGuiTexture, ImGui::GetWindowSize());
						}
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

	// Brush Mask
	GUID_Lambda brushMaskTexID = ResourceManager::GetTextureGUID("MeshPainting/BrushMaskV2.png");

	Texture* pBrushMaskTexture = ResourceManager::GetTexture(brushMaskTexID);
	TextureView* pBrushMaskTextureView = ResourceManager::GetTextureView(brushMaskTexID);

	ResourceUpdateDesc BrushMaskTextureUpdateDesc = {};
	BrushMaskTextureUpdateDesc.ResourceName = "BRUSH_MASK";
	BrushMaskTextureUpdateDesc.ExternalTextureUpdate.ppTextures = &pBrushMaskTexture;
	BrushMaskTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews = &pBrushMaskTextureView;
	BrushMaskTextureUpdateDesc.ExternalTextureUpdate.ppSamplers = &pNearestSampler;

	pRenderGraph->UpdateResource(&BrushMaskTextureUpdateDesc);
}

namespace LambdaEngine
{
	Game* CreateGame(const argh::parser& flagParser)
	{
		UNREFERENCED_VARIABLE(flagParser);
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

	// For Mesh painting in RenderGraph
	{
		GUID_Lambda brushMaskID = ResourceManager::LoadTextureFromFile("MeshPainting/BrushMaskV2.png", EFormat::FORMAT_R8G8B8A8_UNORM, false);

		Texture* pTexture = ResourceManager::GetTexture(brushMaskID);
		TextureView* pTextureView = ResourceManager::GetTextureView(brushMaskID);
		Sampler* pNearestSampler = Sampler::GetNearestSampler();

		ResourceUpdateDesc cubeTextureUpdateDesc = {};
		cubeTextureUpdateDesc.ResourceName = "BRUSH_MASK";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures = &pTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews = &pTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers = &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	RenderSystem::GetInstance().GetRenderGraph()->AddCreateHandler(this);

	return true;
}