#include "Sandbox.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/RenderGraphDescriptionParser.h"

#include "Audio/AudioSystem.h"
#include "Audio/AudioListener.h"
#include "Audio/SoundEffect3D.h"
#include "Audio/SoundInstance3D.h"
#include "Audio/AudioGeometry.h"
#include "Audio/ReverbSphere.h"

#include "Game/Scene.h"

#include "Time/API/Clock.h"

Sandbox::Sandbox()
    : Game(),
	m_pResourceManager(nullptr),
	m_pToneSoundEffect(nullptr),
	m_pToneSoundInstance(nullptr),
	m_pGunSoundEffect(nullptr),
	m_pAudioListener(nullptr),
	m_pReverbSphere(nullptr),
	m_pAudioGeometry(nullptr),
	m_pScene(nullptr)
{
	using namespace LambdaEngine;

	m_pResourceManager = DBG_NEW LambdaEngine::ResourceManager(LambdaEngine::RenderSystem::GetDevice(), LambdaEngine::AudioSystem::GetDevice());

	/*std::vector<GameObject>	sceneGameObjects;
	m_pResourceManager->LoadSceneFromFile("../Assets/Scenes/sponza/", "sponza.obj", sceneGameObjects);

	m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice(), m_pResourceManager);

	for (GameObject& graphicsObject : sceneGameObjects)
	{
		m_pScene->AddDynamicGameObject(graphicsObject, glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)));
	}

	SceneDesc sceneDesc = {};
	m_pScene->Finalize(sceneDesc);*/

	GUID_Lambda blurShaderGUID					= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/blur.spv",					FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,		EShaderLang::SPIRV);

	GUID_Lambda geometryVertexShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/geometryVertex.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	GUID_Lambda geometryPixelShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/geometryPixel.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);

	GUID_Lambda lightVertexShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/lightVertex.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	GUID_Lambda lightPixelShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/lightPixel.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);

	GUID_Lambda raygenRadianceShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/raygenRadiance.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			EShaderLang::SPIRV);
	GUID_Lambda closestHitRadianceShaderGUID	= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/closestHitRadiance.spv",	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::SPIRV);
	GUID_Lambda missRadianceShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/missRadiance.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::SPIRV);
	GUID_Lambda closestHitShadowShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/closestHitShadow.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::SPIRV);
	GUID_Lambda missShadowShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/missShadow.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::SPIRV);

	GUID_Lambda particleUpdateShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/particleUpdate.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,		EShaderLang::SPIRV);

	constexpr uint32 MAX_UNIQUE_MATERIALS = 16;

	std::vector<RenderStageDesc> renderStages;

	GraphicsPipelineStateDesc geometryPassPipelineDesc = {};
	std::vector<RenderStageAttachment>			geometryRenderStageAttachments;

	{
		geometryRenderStageAttachments.push_back({ "PER_FRAME_BUFFER",			EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,	FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});

		geometryRenderStageAttachments.push_back({ "SCENE_MAT_PARAM_BUFFER",	EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,	FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ "SCENE_VERTEX_BUFFER",		EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,	FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ "SCENE_INDEX_BUFFER",		EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,	FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ "SCENE_INSTANCE_BUFFER",		EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,	FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ "SCENE_MESH_INDEX_BUFFER",	EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,	FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});

		geometryRenderStageAttachments.push_back({ "SCENE_ALBEDO_MAPS",			EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, MAX_UNIQUE_MATERIALS});
		geometryRenderStageAttachments.push_back({ "SCENE_NORMAL_MAPS",			EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, MAX_UNIQUE_MATERIALS});
		geometryRenderStageAttachments.push_back({ "SCENE_AO_MAPS",				EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, MAX_UNIQUE_MATERIALS});
		geometryRenderStageAttachments.push_back({ "SCENE_ROUGHNESS_MAPS",		EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, MAX_UNIQUE_MATERIALS});
		geometryRenderStageAttachments.push_back({ "SCENE_METALLIC_MAPS",		EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, MAX_UNIQUE_MATERIALS});

		geometryRenderStageAttachments.push_back({ "GBUFFER_ALBEDO_AO",					EAttachmentType::OUTPUT_COLOR,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1 });
		geometryRenderStageAttachments.push_back({ "GBUFFER_NORMAL_ROUGHNESS_METALLIC",	EAttachmentType::OUTPUT_COLOR,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1 });
		geometryRenderStageAttachments.push_back({ "GBUFFER_VELOCITY",					EAttachmentType::OUTPUT_COLOR,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1 });
		geometryRenderStageAttachments.push_back({ "GBUFFER_DEPTH",						EAttachmentType::OUTPUT_DEPTH_STENCIL,	FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1 });

		RenderStageDesc renderStage = {};
		renderStage.pName						= "Geometry Render Stage";
		renderStage.pAttachments				= geometryRenderStageAttachments.data();
		renderStage.AttachmentCount				= geometryRenderStageAttachments.size();

		geometryPassPipelineDesc.pName				= "Geometry Pass Pipeline State";
		geometryPassPipelineDesc.pVertexShader		= m_pResourceManager->GetShader(geometryVertexShaderGUID);
		geometryPassPipelineDesc.pPixelShader		= m_pResourceManager->GetShader(geometryPixelShaderGUID);

		renderStage.PipelineType					= EPipelineStateType::GRAPHICS;
		renderStage.Pipeline.pGraphicsDesc			= &geometryPassPipelineDesc;

		renderStages.push_back(renderStage);
	}

	RayTracingPipelineStateDesc					rayTracePipelineDesc = {};
	std::vector<IShader*>						rayTraceClosestHitShader;
	std::vector<IShader*>						rayTraceMissShader;
	std::vector<RenderStageAttachment>			rayTraceRenderStageAttachments;

	{
		rayTraceRenderStageAttachments.push_back({ "RADIANCE_IMAGE",					EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			1 });
		rayTraceRenderStageAttachments.push_back({ "GBUFFER_NORMAL_ROUGHNESS_METALLIC",	EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			1 });
		rayTraceRenderStageAttachments.push_back({ "GBUFFER_VELOCITY",					EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			1 });
		rayTraceRenderStageAttachments.push_back({ "GBUFFER_DEPTH",						EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			1 });

		rayTraceRenderStageAttachments.push_back({ "PER_FRAME_BUFFER",					EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });
		rayTraceRenderStageAttachments.push_back({ "LIGHTS_BUFFER",						EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });
		rayTraceRenderStageAttachments.push_back({ "SCENE_TLAS",						EAttachmentType::EXTERNAL_INPUT_ACCELERATION_STRUCTURE,				FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });

		rayTraceRenderStageAttachments.push_back({ "SCENE_MAT_PARAM_BUFFER",			EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });
		rayTraceRenderStageAttachments.push_back({ "SCENE_VERTEX_BUFFER",				EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });
		rayTraceRenderStageAttachments.push_back({ "SCENE_INDEX_BUFFER",				EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });
		rayTraceRenderStageAttachments.push_back({ "SCENE_INSTANCE_BUFFER",				EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });
		rayTraceRenderStageAttachments.push_back({ "SCENE_MESH_INDEX_BUFFER",			EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });

		rayTraceRenderStageAttachments.push_back({ "SCENE_ALBEDO_MAPS",					EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	MAX_UNIQUE_MATERIALS });
		rayTraceRenderStageAttachments.push_back({ "SCENE_NORMAL_MAPS",					EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	MAX_UNIQUE_MATERIALS });
		rayTraceRenderStageAttachments.push_back({ "SCENE_AO_MAPS",						EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	MAX_UNIQUE_MATERIALS });
		rayTraceRenderStageAttachments.push_back({ "SCENE_ROUGHNESS_MAPS",				EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	MAX_UNIQUE_MATERIALS });
		rayTraceRenderStageAttachments.push_back({ "SCENE_METALLIC_MAPS",				EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	MAX_UNIQUE_MATERIALS });

		rayTraceRenderStageAttachments.push_back({ "BRDF_LUT",							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });
		rayTraceRenderStageAttachments.push_back({ "BLUE_NOISE_LUT",					EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	1 });

		rayTraceRenderStageAttachments.push_back({ "RADIANCE_IMAGE",					EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE,					FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			1 });

		RenderStageDesc renderStage = {};
		renderStage.pName						= "Ray Tracing Render Stage";
		renderStage.pAttachments				= rayTraceRenderStageAttachments.data();
		renderStage.AttachmentCount				= rayTraceRenderStageAttachments.size();


		rayTraceClosestHitShader.push_back(m_pResourceManager->GetShader(closestHitRadianceShaderGUID));
		rayTraceClosestHitShader.push_back(m_pResourceManager->GetShader(closestHitShadowShaderGUID));

		rayTraceMissShader.push_back(m_pResourceManager->GetShader(missRadianceShaderGUID));
		rayTraceMissShader.push_back(m_pResourceManager->GetShader(missShadowShaderGUID));

		rayTracePipelineDesc.pName					= "Ray Tracing Pipeline State";
		rayTracePipelineDesc.pRaygenShader			= m_pResourceManager->GetShader(raygenRadianceShaderGUID); 
		rayTracePipelineDesc.ppClosestHitShaders	= rayTraceClosestHitShader.data();
		rayTracePipelineDesc.ClosestHitShaderCount	= rayTraceClosestHitShader.size();
		rayTracePipelineDesc.ppMissShaders			= rayTraceMissShader.data();
		rayTracePipelineDesc.MissShaderCount		= rayTraceMissShader.size();

		renderStage.PipelineType					= EPipelineStateType::RAY_TRACING;
		renderStage.Pipeline.pRayTracingDesc		= &rayTracePipelineDesc;

		renderStages.push_back(renderStage);
	}

	ComputePipelineStateDesc					spatialBlurPipelineDesc = {};
	std::vector<RenderStageAttachment>			spatialBlurRenderStageAttachments;

	{
		spatialBlurRenderStageAttachments.push_back({ "RADIANCE_IMAGE",								EAttachmentType::INPUT_UNORDERED_ACCESS_TEXTURE,					FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,			1 });

		spatialBlurRenderStageAttachments.push_back({ "FILTERED_RADIANCE_IMAGE",					EAttachmentType::OUTPUT_UNORDERED_ACCESS_TEXTURE,					FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,			1 });

		RenderStageDesc renderStage = {};
		renderStage.pName						= "Spatial Blur Render Stage";
		renderStage.pAttachments				= spatialBlurRenderStageAttachments.data();
		renderStage.AttachmentCount				= spatialBlurRenderStageAttachments.size();

		spatialBlurPipelineDesc.pName			= "Spatial Blur Pipeline State";
		spatialBlurPipelineDesc.pShader			= m_pResourceManager->GetShader(blurShaderGUID);

		renderStage.PipelineType				= EPipelineStateType::COMPUTE;
		renderStage.Pipeline.pComputeDesc		= &spatialBlurPipelineDesc;

		renderStages.push_back(renderStage);
	}

	ComputePipelineStateDesc					particleUpdatePipelineDesc = {};
	std::vector<RenderStageAttachment>			particleUpdateRenderStageAttachments;

	{
		particleUpdateRenderStageAttachments.push_back({ "PARTICLE_BUFFER",					EAttachmentType::INPUT_UNORDERED_ACCESS_BUFFER,								FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,			1 });

		particleUpdateRenderStageAttachments.push_back({ "PARTICLE_BUFFER",					EAttachmentType::OUTPUT_UNORDERED_ACCESS_BUFFER,							FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,			1 });

		RenderStageDesc renderStage = {};
		renderStage.pName						= "Particle Update Render Stage";
		renderStage.pAttachments				= particleUpdateRenderStageAttachments.data();
		renderStage.AttachmentCount				= particleUpdateRenderStageAttachments.size();

		particleUpdatePipelineDesc.pName	= "Particle Update Pipeline State";
		particleUpdatePipelineDesc.pShader	= m_pResourceManager->GetShader(particleUpdateShaderGUID);

		renderStage.PipelineType			= EPipelineStateType::COMPUTE;
		renderStage.Pipeline.pComputeDesc	= &particleUpdatePipelineDesc;

		renderStages.push_back(renderStage);
	}

	GraphicsPipelineStateDesc					shadingPipelineDesc = {};
	std::vector<RenderStageAttachment>			shadingRenderStageAttachments;

	{
		shadingRenderStageAttachments.push_back({ "FILTERED_RADIANCE_IMAGE",					EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });
		shadingRenderStageAttachments.push_back({ "GBUFFER_ALBEDO_AO",							EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });
		shadingRenderStageAttachments.push_back({ "GBUFFER_NORMAL_ROUGHNESS_METALLIC",			EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });
		shadingRenderStageAttachments.push_back({ "GBUFFER_VELOCITY",							EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });
		shadingRenderStageAttachments.push_back({ "GBUFFER_DEPTH",								EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });

		shadingRenderStageAttachments.push_back({ "PER_FRAME_BUFFER",							EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });
		shadingRenderStageAttachments.push_back({ "LIGHTS_BUFFER",								EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });

		shadingRenderStageAttachments.push_back({ "BRDF_LUT",									EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });
		shadingRenderStageAttachments.push_back({ "BLUE_NOISE_LUT",								EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });

		shadingRenderStageAttachments.push_back({ RENDER_GRAPH_BACK_BUFFER,						EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			1 });

		RenderStageDesc renderStage = {};
		renderStage.pName						= "Shading Render Stage";
		renderStage.pAttachments				= shadingRenderStageAttachments.data();
		renderStage.AttachmentCount				= shadingRenderStageAttachments.size();

		shadingPipelineDesc.pName				= "Shading Pass Pipeline State";
		shadingPipelineDesc.pVertexShader		= m_pResourceManager->GetShader(lightVertexShaderGUID);
		shadingPipelineDesc.pPixelShader		= m_pResourceManager->GetShader(lightPixelShaderGUID);

		renderStage.PipelineType				= EPipelineStateType::GRAPHICS;
		renderStage.Pipeline.pGraphicsDesc		= &shadingPipelineDesc;

		renderStages.push_back(renderStage);
	}

	RenderGraphDesc renderGraphDesc = {};
	renderGraphDesc.pName				= "Test Render Graph";
	renderGraphDesc.CreateDebugGraph	= true;
	renderGraphDesc.pRenderStages		= renderStages.data();
	renderGraphDesc.RenderStageCount	= renderStages.size();

	LambdaEngine::Clock clock;
	clock.Reset();
	clock.Tick();

	RenderGraph* pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());

	//pRenderGraph->Init(renderGraphDesc);

	clock.Tick();
	LOG_INFO("Render Graph Build Time: %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

	SAFEDELETE(pRenderGraph);

	//InitTestAudio();
}

Sandbox::~Sandbox()
{
	SAFEDELETE(m_pResourceManager);
	SAFEDELETE(m_pAudioListener);
	SAFEDELETE(m_pAudioGeometry);

	SAFEDELETE(m_pScene);
}

void Sandbox::InitTestAudio()
{
	using namespace LambdaEngine;

	m_ToneSoundEffectGUID = m_pResourceManager->LoadSoundFromFile("../Assets/Sounds/noise.wav");
	m_GunSoundEffectGUID = m_pResourceManager->LoadSoundFromFile("../Assets/Sounds/GUN_FIRE-GoodSoundForYou.wav");

	m_pToneSoundEffect = m_pResourceManager->GetSound(m_ToneSoundEffectGUID);
	m_pGunSoundEffect = m_pResourceManager->GetSound(m_GunSoundEffectGUID);

	SoundInstance3DDesc soundInstanceDesc = {};
	soundInstanceDesc.pSoundEffect = m_pToneSoundEffect;
	soundInstanceDesc.Flags = FSoundModeFlags::SOUND_MODE_LOOPING;

	m_pToneSoundInstance = AudioSystem::GetDevice()->CreateSoundInstance();
	m_pToneSoundInstance->Init(soundInstanceDesc);
	m_pToneSoundInstance->SetVolume(0.5f);

	m_SpawnPlayAts = false;
	m_GunshotTimer = 0.0f;
	m_GunshotDelay = 1.0f;
	m_Timer = 0.0f;

	AudioSystem::GetDevice()->LoadMusic("../Assets/Sounds/halo_theme.ogg");

	m_pAudioListener = AudioSystem::GetDevice()->CreateAudioListener();
	m_pAudioListener->Update(glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	m_pReverbSphere = AudioSystem::GetDevice()->CreateReverbSphere();

	ReverbSphereDesc reverbSphereDesc = {};
	reverbSphereDesc.Position = glm::vec3(0.0f, 0.0f, 5.0f);
	reverbSphereDesc.MinDistance = 20.0f;
	reverbSphereDesc.MaxDistance = 40.0f;
	reverbSphereDesc.ReverbSetting = EReverbSetting::SEWERPIPE;

	m_pReverbSphere->Init(reverbSphereDesc);

	m_pAudioGeometry = AudioSystem::GetDevice()->CreateAudioGeometry();

	GUID_Lambda sphereGUID = m_pResourceManager->LoadMeshFromFile("../Assets/Meshes/sphere.obj");
	Mesh* sphereMesh = m_pResourceManager->GetMesh(sphereGUID);
	glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
	AudioMeshParameters audioMeshParameters = {};
	audioMeshParameters.DirectOcclusion = 0.0f;
	audioMeshParameters.ReverbOcclusion = 0.25f;
	audioMeshParameters.DoubleSided = true;

	AudioGeometryDesc audioGeometryDesc = {};
	audioGeometryDesc.NumMeshes = 1;
	audioGeometryDesc.ppMeshes = &sphereMesh;
	audioGeometryDesc.pTransforms = &transform;
	audioGeometryDesc.pAudioMeshParameters = &audioMeshParameters;

	m_pAudioGeometry->Init(audioGeometryDesc);

	/*std::vector<GraphicsObject> sponzaGraphicsObjects;
	m_pResourceManager->LoadSceneFromFile("../Assets/Scenes/sponza/", "sponza.obj", sponzaGraphicsObjects);

	std::vector<Mesh*> sponzaMeshes;
	std::vector<glm::mat4> sponzaMeshTransforms;
	std::vector<LambdaEngine::AudioMeshParameters> sponzaAudioMeshParameters;

	for (GraphicsObject& graphicsObject : sponzaGraphicsObjects)
	{
		sponzaMeshes.push_back(m_pResourceManager->GetMesh(graphicsObject.Mesh));
		sponzaMeshTransforms.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(0.001f)));

		LambdaEngine::AudioMeshParameters audioMeshParameters = {};
		audioMeshParameters.DirectOcclusion = 1.0f;
		audioMeshParameters.ReverbOcclusion = 1.0f;
		audioMeshParameters.DoubleSided = true;
		sponzaAudioMeshParameters.push_back(audioMeshParameters);
	}

	AudioGeometryDesc audioGeometryDesc = {};
	audioGeometryDesc.pName = "Test";
	audioGeometryDesc.NumMeshes = sponzaMeshes.size();
	audioGeometryDesc.ppMeshes = sponzaMeshes.data();
	audioGeometryDesc.pTransforms = sponzaMeshTransforms.data();
	audioGeometryDesc.pAudioMeshParameters = sponzaAudioMeshParameters.data();

	m_pAudioGeometry->Init(audioGeometryDesc);*/
}

void Sandbox::OnKeyDown(LambdaEngine::EKey key)
{
	//LOG_MESSAGE("Key Pressed: %d", key);

	using namespace LambdaEngine;

	static bool geometryAudioActive = true;
	static bool reverbSphereActive = true;

	if (key == EKey::KEY_KP_1)
	{
		m_pToneSoundInstance->Toggle();
	}
	else if (key == EKey::KEY_KP_2)
	{
		m_SpawnPlayAts = !m_SpawnPlayAts;
	}
	else if (key == EKey::KEY_KP_3)
	{
		m_pGunSoundEffect->PlayOnceAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), 1.0f);
	}
	else if (key == EKey::KEY_KP_ADD)
	{
		m_GunshotDelay += 0.05f;
	}
	else if (key == EKey::KEY_KP_SUBTRACT)
	{
		m_GunshotDelay = glm::max(m_GunshotDelay - 0.05f, 0.125f);
	}
	else if (key == EKey::KEY_KP_5)
	{
		AudioSystem::GetDevice()->ToggleMusic();
	}
	else if (key == EKey::KEY_KP_7)
	{
		if (m_pAudioGeometry != nullptr)
		{
			geometryAudioActive = !geometryAudioActive;
			LOG_MESSAGE("AudioGeometry %s", geometryAudioActive ? "Enabled" : "Disabled");
			m_pAudioGeometry->SetActive(geometryAudioActive);
		}
	}
	else if (key == EKey::KEY_KP_8)
	{
		if (m_pReverbSphere != nullptr)
		{
			reverbSphereActive = !reverbSphereActive;
			LOG_MESSAGE("ReverbSphere %s", reverbSphereActive ? "Enabled" : "Disabled");
			m_pReverbSphere->SetActive(reverbSphereActive);
		}
	}
}

void Sandbox::OnKeyHeldDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Held Down: %d", key);
}

void Sandbox::OnKeyUp(LambdaEngine::EKey key)
{
	//LOG_MESSAGE("Key Released: %d", key);
}

void Sandbox::OnMouseMove(int32 x, int32 y)
{
	//LOG_MESSAGE("Mouse Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button)
{
	LOG_MESSAGE("Mouse Button Pressed: %d", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	LOG_MESSAGE("Mouse Button Released: %d", button);
}

void Sandbox::OnScroll(int32 delta)
{
	//LOG_MESSAGE("Mouse Scrolled: %d", delta);
}

void Sandbox::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

    //LOG_MESSAGE("Delta: %.6f ms", delta.AsMilliSeconds());
    
	m_Timer += delta.AsSeconds();

	if (m_pGunSoundEffect != nullptr)
	{
		if (m_SpawnPlayAts)
		{
			m_GunshotTimer += delta.AsSeconds();

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
}

void Sandbox::FixedTick(LambdaEngine::Timestamp delta)
{
    //LOG_MESSAGE("Fixed delta: %.6f ms", delta.AsMilliSeconds());
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
        Sandbox* pSandbox = DBG_NEW Sandbox();
        Input::AddKeyboardHandler(pSandbox);
        Input::AddMouseHandler(pSandbox);
        
        return pSandbox;
    }
}
