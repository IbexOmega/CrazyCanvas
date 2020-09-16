#include "CrazyCanvas.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

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
#include "Rendering/ImGuiRenderer.h"

#include "Audio/AudioSystem.h"
#include "Audio/API/ISoundEffect3D.h"
#include "Audio/API/ISoundInstance3D.h"
#include "Audio/API/IAudioGeometry.h"
#include "Audio/API/IReverbSphere.h"
#include "Audio/API/IMusic.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Engine/EngineConfig.h"

#include "Time/API/Clock.h"

#include "Threading/API/Thread.h"

#include "Utilities/RuntimeStats.h"

#include "Game/StateManager.h"
#include "States/DebugState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include <imgui.h>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>


constexpr const uint32 NUM_BLUE_NOISE_LUTS = 128;

CrazyCanvas::CrazyCanvas()
{
	using namespace LambdaEngine;

	Input::Disable();

	GraphicsDeviceFeatureDesc deviceFeatures = {};
	RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);

	m_pCamera = DBG_NEW Camera();

	TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= 90.0f;
	cameraDesc.Width		= window->GetWidth();
	cameraDesc.Height		= window->GetHeight();
	cameraDesc.NearPlane	= 0.001f;
	cameraDesc.FarPlane		= 1000.0f;

	m_pCamera->Init(cameraDesc);
	m_pCamera->Update();

	std::vector<glm::vec3> cameraTrack = {
		{-2.0f, 1.6f, 1.0f},
		{9.8f, 1.6f, 0.8f},
		{9.4f, 1.6f, -3.8f},
		{-9.8f, 1.6f, -3.9f},
		{-11.6f, 1.6f, -1.1f},
		{9.8f, 6.1f, -0.8f},
		{9.4f, 6.1f, 3.8f},
		{-9.8f, 6.1f, 3.9f}
	};

	LoadRendererResources();

	m_CameraTrack.Init(m_pCamera, cameraTrack);
	StateManager::GetInstance()->EnqueueStateTransition(DBG_NEW(DebugState), STATE_TRANSITION::PUSH);

	RenderSystem::GetInstance().SetCamera(m_pCamera);

}

CrazyCanvas::~CrazyCanvas()
{
	SAFEDELETE(m_pCamera);
}

void CrazyCanvas::Tick(LambdaEngine::Timestamp delta)
{
	if (m_CameraTrack.HasReachedEnd())
	{
		PrintBenchmarkResults();
		LambdaEngine::CommonApplication::Get()->Terminate();
		return;
	}

	Render(delta);
}

void CrazyCanvas::FixedTick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	float32 dt = (float32)delta.AsSeconds();

	m_pCamera->Update();
	m_CameraTrack.Tick(dt);
}

void CrazyCanvas::Render(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;
}

namespace LambdaEngine
{
	Game* CreateGame()
	{
		return DBG_NEW CrazyCanvas();
	}
}

bool CrazyCanvas::LoadRendererResources()
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

		Texture* pBlueNoiseTexture = ResourceManager::GetTexture(blueNoiseID);
		TextureView* pBlueNoiseTextureView = ResourceManager::GetTextureView(blueNoiseID);

		Sampler* pNearestSampler = Sampler::GetNearestSampler();

		ResourceUpdateDesc blueNoiseUpdateDesc = {};
		blueNoiseUpdateDesc.ResourceName = "BLUE_NOISE_LUT";
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures = &pBlueNoiseTexture;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews = &pBlueNoiseTextureView;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers = &pNearestSampler;

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
		cubeTextureUpdateDesc.ResourceName							= "SKYBOX";
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextures		= &pCubeTexture;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppTextureViews	= &pCubeTextureView;
		cubeTextureUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pNearestSampler;

		RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&cubeTextureUpdateDesc);
	}

	return true;
}

void CrazyCanvas::PrintBenchmarkResults()
{
	using namespace rapidjson;
	using namespace LambdaEngine;

	constexpr const float MB = 1000000.0f;

	StringBuffer jsonStringBuffer;
	PrettyWriter<StringBuffer> writer(jsonStringBuffer);

	writer.StartObject();

	writer.String("AverageFPS");
	writer.Double(1.0f / RuntimeStats::GetAverageFrametime());
	writer.String("PeakMemoryUsage");
	writer.Double(RuntimeStats::GetPeakMemoryUsage() / MB);

	writer.EndObject();

	FILE* pFile = fopen("benchmark_results.json", "w");

	if (pFile)
	{
		fputs(jsonStringBuffer.GetString(), pFile);
		fclose(pFile);
	}
}
