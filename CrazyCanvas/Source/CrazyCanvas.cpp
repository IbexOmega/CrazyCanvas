#include "CrazyCanvas.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/Renderer.h"
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

#include "Game/Scene.h"

#include "Time/API/Clock.h"

#include "Threading/API/Thread.h"

#include "Utilities/RuntimeStats.h"

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
	CommonApplication::Get()->AddEventHandler(this);

	m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice());

	GraphicsDeviceFeatureDesc deviceFeatures = {};
	RenderSystem::GetDevice()->QueryDeviceFeatures(&deviceFeatures);

	SceneDesc sceneDesc = { };
	sceneDesc.Name				= "Test Scene";
	sceneDesc.RayTracingEnabled = deviceFeatures.RayTracing && EngineConfig::GetBoolProperty("RayTracingEnabled");
	m_pScene->Init(sceneDesc);

	DirectionalLight directionalLight;
	directionalLight.Direction			= glm::vec4(glm::normalize(glm::vec3(glm::cos(glm::half_pi<float>()), glm::sin(glm::half_pi<float>()), 0.0f)), 0.0f);
	directionalLight.EmittedRadiance	= glm::vec4(10.0f, 10.0f, 10.0f, 0.0f);

	m_pScene->SetDirectionalLight(directionalLight);

	AreaLightObject areaLight;
	areaLight.Type = EAreaLightType::QUAD;
	areaLight.Material = GUID_MATERIAL_DEFAULT_EMISSIVE;

	//Lights
	{
		glm::vec3 position(0.0f, 6.0f, 0.0f);
		glm::vec4 rotation(1.0f, 0.0f, 0.0f, glm::pi<float>());
		glm::vec3 scale(1.5f);

		glm::mat4 transform(1.0f);
		transform = glm::translate(transform, position);
		transform = glm::rotate(transform, rotation.w, glm::vec3(rotation));
		transform = glm::scale(transform, scale);

		m_pScene->AddAreaLight(areaLight, transform);
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
			m_pScene->AddDynamicGameObject(gameObject, transform);
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

	m_pCamera->Init(CommonApplication::Get(), cameraDesc);
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

	m_CameraTrack.Init(m_pCamera, cameraTrack);

	LoadRendererResources();
	m_pScene->UpdateCamera(m_pCamera);
}

CrazyCanvas::~CrazyCanvas()
{
	LambdaEngine::CommonApplication::Get()->RemoveEventHandler(this);

	SAFEDELETE(m_pScene);
	SAFEDELETE(m_pCamera);
}

void CrazyCanvas::Tick(LambdaEngine::Timestamp delta)
{
	if (m_CameraTrack.hasReachedEnd())
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
	m_pScene->UpdateCamera(m_pCamera);
}

void CrazyCanvas::Render(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	Renderer::NewFrame(delta);
	Renderer::PrepareRender(delta);
	Renderer::Render();
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

		Renderer::GetRenderGraph()->UpdateResource(blueNoiseUpdateDesc);
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
