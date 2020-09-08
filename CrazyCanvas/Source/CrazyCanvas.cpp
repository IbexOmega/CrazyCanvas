#include "CrazyCanvas.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/Renderer.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraphEditor.h"
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


constexpr const uint32 BACK_BUFFER_COUNT = 3;
#ifdef LAMBDA_PLATFORM_MACOS
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 8;
#else
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 256;
#endif
constexpr const bool SHOW_DEMO					= true;
constexpr const bool SVGF_ENABLED				= false;

constexpr const bool RENDER_GRAPH_IMGUI_ENABLED	= false;
constexpr const bool RENDERING_DEBUG_ENABLED	= false;

constexpr const float DEFAULT_DIR_LIGHT_R			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_G			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_B			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_STRENGTH	= 0.0f;

constexpr const uint32 NUM_BLUE_NOISE_LUTS = 128;

CrazyCanvas::CrazyCanvas()
{
	using namespace LambdaEngine;

	Input::Disable();
	CommonApplication::Get()->AddEventHandler(this);

	m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice());

	SceneDesc sceneDesc = {};
	sceneDesc.Name				= "Benchmark Scene";
	sceneDesc.RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
	m_pScene->Init(sceneDesc);

	m_DirectionalLightAngle	= glm::half_pi<float>();
	m_DirectionalLightStrength[0] = DEFAULT_DIR_LIGHT_R;
	m_DirectionalLightStrength[1] = DEFAULT_DIR_LIGHT_G;
	m_DirectionalLightStrength[2] = DEFAULT_DIR_LIGHT_B;
	m_DirectionalLightStrength[3] = DEFAULT_DIR_LIGHT_STRENGTH;

	DirectionalLight directionalLight;
	directionalLight.Direction			= glm::vec4(glm::normalize(glm::vec3(glm::cos(m_DirectionalLightAngle), glm::sin(m_DirectionalLightAngle), 0.0f)), 0.0f);
	directionalLight.EmittedRadiance	= glm::vec4(glm::vec3(m_DirectionalLightStrength[0], m_DirectionalLightStrength[1], m_DirectionalLightStrength[2]) * m_DirectionalLightStrength[3], 0.0f);

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

	m_pScene->Finalize();

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

	SamplerDesc samplerLinearDesc = {};
	samplerLinearDesc.DebugName				= "Linear Sampler";
	samplerLinearDesc.MinFilter				= EFilterType::FILTER_TYPE_LINEAR;
	samplerLinearDesc.MagFilter				= EFilterType::FILTER_TYPE_LINEAR;
	samplerLinearDesc.MipmapMode			= EMipmapMode::MIPMAP_MODE_LINEAR;
	samplerLinearDesc.AddressModeU			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	samplerLinearDesc.AddressModeV			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	samplerLinearDesc.AddressModeW			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	samplerLinearDesc.MipLODBias			= 0.0f;
	samplerLinearDesc.AnisotropyEnabled		= false;
	samplerLinearDesc.MaxAnisotropy			= 16;
	samplerLinearDesc.MinLOD				= 0.0f;
	samplerLinearDesc.MaxLOD				= 1.0f;

	m_pLinearSampler = RenderSystem::GetDevice()->CreateSampler(&samplerLinearDesc);

	SamplerDesc samplerNearestDesc = {};
	samplerNearestDesc.DebugName			= "Nearest Sampler";
	samplerNearestDesc.MinFilter			= EFilterType::FILTER_TYPE_NEAREST;
	samplerNearestDesc.MagFilter			= EFilterType::FILTER_TYPE_NEAREST;
	samplerNearestDesc.MipmapMode			= EMipmapMode::MIPMAP_MODE_NEAREST;
	samplerNearestDesc.AddressModeU			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	samplerNearestDesc.AddressModeV			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	samplerNearestDesc.AddressModeW			= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_REPEAT;
	samplerNearestDesc.MipLODBias			= 0.0f;
	samplerNearestDesc.AnisotropyEnabled	= false;
	samplerNearestDesc.MaxAnisotropy		= 16;
	samplerNearestDesc.MinLOD				= 0.0f;
	samplerNearestDesc.MaxLOD				= 1.0f;

	m_pNearestSampler = RenderSystem::GetDevice()->CreateSampler(&samplerNearestDesc);

	m_pRenderGraphEditor = DBG_NEW RenderGraphEditor();

	InitRendererForDeferred();

	CommandList* pGraphicsCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();
	CommandList* pComputeCopyCommandList = m_pRenderer->AcquireComputeCopyCommandList();

	m_pScene->UpdateCamera(m_pCamera);

	//InitTestAudio();
}

CrazyCanvas::~CrazyCanvas()
{
	LambdaEngine::CommonApplication::Get()->RemoveEventHandler(this);

	SAFEDELETE(m_pAudioGeometry);

	SAFEDELETE(m_pScene);
	SAFEDELETE(m_pCamera);
	SAFERELEASE(m_pLinearSampler);
	SAFERELEASE(m_pNearestSampler);

	SAFEDELETE(m_pRenderGraph);
	SAFEDELETE(m_pRenderer);

	SAFEDELETE(m_pRenderGraphEditor);
}

void CrazyCanvas::InitTestAudio()
{
	using namespace LambdaEngine;

	MusicDesc musicDesc = {};
	musicDesc.pFilepath		= "../Assets/Sounds/halo_theme.ogg";
	musicDesc.Volume		= 0.5f;
	musicDesc.Pitch			= 1.0f;

	AudioSystem::GetDevice()->CreateMusic(&musicDesc);
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

	float dt = (float)delta.AsSeconds();
	m_Timer += dt;

	if (m_pGunSoundEffect != nullptr)
	{
		if (m_SpawnPlayAts)
		{
			m_GunshotTimer += dt;

			if (m_GunshotTimer > m_GunshotDelay)
			{

				glm::vec3 gunPosition(glm::cos(m_Timer), 0.0f, glm::sin(m_Timer));
				m_pGunSoundEffect->PlayOnceAt(gunPosition, glm::vec3(0.0f), 0.5f);
				m_GunshotTimer = 0.0f;
			}
		}
	}

	if (m_pToneSoundInstance != nullptr)
	{
		glm::vec3 tonePosition(glm::cos(m_Timer), 0.0f, glm::sin(m_Timer));
		m_pToneSoundInstance->SetPosition(tonePosition);
	}

	constexpr float CAMERA_MOVEMENT_SPEED = 1.4f;
	constexpr float CAMERA_ROTATION_SPEED = 45.0f;

	if (Input::IsKeyDown(EKey::KEY_W) && Input::IsKeyUp(EKey::KEY_S))
	{
		m_pCamera->Translate(glm::vec3(0.0f, 0.0f, CAMERA_MOVEMENT_SPEED * delta.AsSeconds()));
	}
	else if (Input::IsKeyDown(EKey::KEY_S) && Input::IsKeyUp(EKey::KEY_W))
	{
		m_pCamera->Translate(glm::vec3(0.0f, 0.0f, -CAMERA_MOVEMENT_SPEED * delta.AsSeconds()));
	}

	if (Input::IsKeyDown(EKey::KEY_A) && Input::IsKeyUp(EKey::KEY_D))
	{
		m_pCamera->Translate(glm::vec3(-CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_D) && Input::IsKeyUp(EKey::KEY_A))
	{
		m_pCamera->Translate(glm::vec3(CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}

	if (Input::IsKeyDown(EKey::KEY_Q) && Input::IsKeyUp(EKey::KEY_E))
	{
		m_pCamera->Translate(glm::vec3(0.0f, CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_E) && Input::IsKeyUp(EKey::KEY_Q))
	{
		m_pCamera->Translate(glm::vec3(0.0f, -CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f));
	}

	if (Input::IsKeyDown(EKey::KEY_UP) && Input::IsKeyUp(EKey::KEY_DOWN))
	{
		m_pCamera->Rotate(glm::vec3(-CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_DOWN) && Input::IsKeyUp(EKey::KEY_UP))
	{
		m_pCamera->Rotate(glm::vec3(CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}

	if (Input::IsKeyDown(EKey::KEY_LEFT) && Input::IsKeyUp(EKey::KEY_RIGHT))
	{
		m_pCamera->Rotate(glm::vec3(0.0f, -CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f));
	}
	else if (Input::IsKeyDown(EKey::KEY_RIGHT) && Input::IsKeyUp(EKey::KEY_LEFT))
	{
		m_pCamera->Rotate(glm::vec3(0.0f, CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f));
	}

	m_pCamera->Update();
	m_CameraTrack.Tick(dt);
	m_pScene->UpdateCamera(m_pCamera);

	AudioListenerDesc listenerDesc = {};
	listenerDesc.Position = m_pCamera->GetPosition();
	listenerDesc.Forward = m_pCamera->GetForwardVec();
	listenerDesc.Up = m_pCamera->GetUpVec();

	AudioSystem::GetDevice()->UpdateAudioListener(m_AudioListenerIndex, &listenerDesc);
}

void CrazyCanvas::Render(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	m_pRenderGraph->Update();

	m_pRenderer->NewFrame(delta);

	CommandList* pGraphicsCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();
	CommandList* pComputeCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();

	m_pScene->PrepareRender(pGraphicsCopyCommandList, pComputeCopyCommandList, m_pRenderer->GetFrameIndex(), delta);
	m_pRenderer->PrepareRender(delta);

	m_pRenderer->Render();
}

namespace LambdaEngine
{
	Game* CreateGame()
	{
		return DBG_NEW CrazyCanvas();
	}
}

bool CrazyCanvas::InitRendererForDeferred()
{
	using namespace LambdaEngine;

	const bool rayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
	String renderGraphFile = "";
	if (SHOW_DEMO)
	{
		renderGraphFile = "../Assets/RenderGraphs/DEMO.lrg";
		//renderGraphFile = "../Assets/RenderGraphs/SIMPLE_RASTERIZER_PBR.lrg";
	}
	else
	{
		if (rayTracingEnabled && !SVGF_ENABLED)
		{
			renderGraphFile = "../Assets/RenderGraphs/TRT_DEFERRED_SIMPLE.lrg";
		}
		else if (rayTracingEnabled && SVGF_ENABLED)
		{
			renderGraphFile = "../Assets/RenderGraphs/TRT_DEFERRED_SVGF.lrg";
		}
	}

	RenderGraphStructureDesc renderGraphStructure = m_pRenderGraphEditor->CreateRenderGraphStructure(renderGraphFile, RENDER_GRAPH_IMGUI_ENABLED);

	RenderGraphDesc renderGraphDesc = {};
	renderGraphDesc.pRenderGraphStructureDesc	= &renderGraphStructure;
	renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;
	renderGraphDesc.MaxTexturesPerDescriptorSet = MAX_TEXTURES_PER_DESCRIPTOR_SET;
	renderGraphDesc.pScene						= m_pScene;

	LambdaEngine::Clock clock;
	clock.Reset();
	clock.Tick();

	m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());
	m_pRenderGraph->Init(&renderGraphDesc);

	clock.Tick();
	LOG_INFO("Render Graph Build Time: %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

	SamplerDesc nearestSamplerDesc		= m_pNearestSampler->GetDesc();
	SamplerDesc linearSamplerDesc		= m_pLinearSampler->GetDesc();

	SamplerDesc* pNearestSamplerDesc	= &nearestSamplerDesc;
	SamplerDesc* pLinearSamplerDesc		= &linearSamplerDesc;

	TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
	uint32 renderWidth	= mainWindow->GetWidth();
	uint32 renderHeight = mainWindow->GetHeight();

	{
		Buffer* pBuffer = m_pScene->GetLightsBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_LIGHTS_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		Buffer* pBuffer = m_pScene->GetPerFrameBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= PER_FRAME_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		Buffer* pBuffer = m_pScene->GetMaterialProperties();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_MAT_PARAM_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		Buffer* pBuffer = m_pScene->GetVertexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_VERTEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		Buffer* pBuffer = m_pScene->GetIndexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_INDEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		Buffer* pBuffer = m_pScene->GetPrimaryInstanceBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_PRIMARY_INSTANCE_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		Buffer* pBuffer = m_pScene->GetSecondaryInstanceBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_SECONDARY_INSTANCE_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		Buffer* pBuffer = m_pScene->GetIndirectArgsBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= SCENE_INDIRECT_ARGS_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		Texture** ppAlbedoMaps						= m_pScene->GetAlbedoMaps();
		Texture** ppNormalMaps						= m_pScene->GetNormalMaps();
		Texture** ppAmbientOcclusionMaps			= m_pScene->GetAmbientOcclusionMaps();
		Texture** ppMetallicMaps					= m_pScene->GetMetallicMaps();
		Texture** ppRoughnessMaps					= m_pScene->GetRoughnessMaps();

		TextureView** ppAlbedoMapViews				= m_pScene->GetAlbedoMapViews();
		TextureView** ppNormalMapViews				= m_pScene->GetNormalMapViews();
		TextureView** ppAmbientOcclusionMapViews	= m_pScene->GetAmbientOcclusionMapViews();
		TextureView** ppMetallicMapViews			= m_pScene->GetMetallicMapViews();
		TextureView** ppRoughnessMapViews			= m_pScene->GetRoughnessMapViews();

		std::vector<Sampler*> linearSamplers(MAX_UNIQUE_MATERIALS, m_pLinearSampler);
		std::vector<Sampler*> nearestSamplers(MAX_UNIQUE_MATERIALS, m_pNearestSampler);

		ResourceUpdateDesc albedoMapsUpdateDesc = {};
		albedoMapsUpdateDesc.ResourceName								= SCENE_ALBEDO_MAPS;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppAlbedoMaps;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppAlbedoMapViews;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

		ResourceUpdateDesc normalMapsUpdateDesc = {};
		normalMapsUpdateDesc.ResourceName								= SCENE_NORMAL_MAPS;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppNormalMaps;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppNormalMapViews;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

		ResourceUpdateDesc aoMapsUpdateDesc = {};
		aoMapsUpdateDesc.ResourceName									= SCENE_AO_MAPS;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= ppAmbientOcclusionMaps;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= ppAmbientOcclusionMapViews;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= nearestSamplers.data();

		ResourceUpdateDesc metallicMapsUpdateDesc = {};
		metallicMapsUpdateDesc.ResourceName								= SCENE_METALLIC_MAPS;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppMetallicMaps;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppMetallicMapViews;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= nearestSamplers.data();

		ResourceUpdateDesc roughnessMapsUpdateDesc = {};
		roughnessMapsUpdateDesc.ResourceName							= SCENE_ROUGHNESS_MAPS;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= ppRoughnessMaps;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= ppRoughnessMapViews;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= nearestSamplers.data();

		m_pRenderGraph->UpdateResource(albedoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(normalMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(aoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(metallicMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(roughnessMapsUpdateDesc);
	}

	if (rayTracingEnabled)
	{
		const AccelerationStructure* pTLAS = m_pScene->GetTLAS();
		ResourceUpdateDesc resourceUpdateDesc					= {};
		resourceUpdateDesc.ResourceName							= SCENE_TLAS;
		resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= pTLAS;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		String blueNoiseLUTFileNames[NUM_BLUE_NOISE_LUTS];

		for (uint32 i = 0; i < NUM_BLUE_NOISE_LUTS; i++)
		{
			char str[5];
			snprintf(str, 5, "%04d", i);
			blueNoiseLUTFileNames[i] = "LUTs\\BlueNoise\\256_256\\HDR_RGBA_" + std::string(str) + ".png";
		}

		GUID_Lambda blueNoiseID = ResourceManager::LoadTextureArrayFromFile("Blue Noise Texture", blueNoiseLUTFileNames, NUM_BLUE_NOISE_LUTS, EFormat::FORMAT_R16_UNORM, false);

		Texture* pBlueNoiseTexture				= ResourceManager::GetTexture(blueNoiseID);
		TextureView* pBlueNoiseTextureView		= ResourceManager::GetTextureView(blueNoiseID);

		ResourceUpdateDesc blueNoiseUpdateDesc = {};
		blueNoiseUpdateDesc.ResourceName								= "BLUE_NOISE_LUT";
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextures			= &pBlueNoiseTexture;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppTextureViews		= &pBlueNoiseTextureView;
		blueNoiseUpdateDesc.ExternalTextureUpdate.ppSamplers			= &m_pNearestSampler;

		m_pRenderGraph->UpdateResource(blueNoiseUpdateDesc);
	}

	m_pRenderer = DBG_NEW Renderer(RenderSystem::GetDevice());

	RendererDesc rendererDesc = {};
	rendererDesc.Name				= "Renderer";
	rendererDesc.Debug				= RENDERING_DEBUG_ENABLED;
	rendererDesc.pRenderGraph		= m_pRenderGraph;
	rendererDesc.pWindow			= CommonApplication::Get()->GetMainWindow().Get();
	rendererDesc.BackBufferCount	= BACK_BUFFER_COUNT;

	m_pRenderer->Init(&rendererDesc);

	if (RENDERING_DEBUG_ENABLED)
	{
		ImGui::SetCurrentContext(ImGuiRenderer::GetImguiContext());
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
