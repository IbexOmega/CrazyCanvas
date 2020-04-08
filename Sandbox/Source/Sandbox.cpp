#include "Sandbox.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Network/API/PlatformNetworkUtils.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/RenderGraphCreator.h"

#include "Audio/AudioSystem.h"
#include "Audio/AudioListener.h"
#include "Audio/SoundEffect3D.h"
#include "Audio/SoundInstance3D.h"
#include "Audio/AudioGeometry.h"
#include "Audio/ReverbSphere.h"

#include "Game/Scene.h"

#include "Time/API/Clock.h"

Sandbox::Sandbox() : 
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

	constexpr uint32 MAX_UNIQUE_MATERIALS = 16;

	std::vector<RenderStage> renderStages;

	std::vector<RenderStageInputAttachment>			geometryRenderStageInputAttachment;
	std::vector<RenderStageExternalInputAttachment> geometryRenderStageExternalInputAttachment;
	std::vector<RenderStageOutputAttachment>		geometryRenderStageOutputAttachment;

	{
		geometryRenderStageExternalInputAttachment.push_back({ "PER_FRAME_BUFFER",				EExternalInputAttachmentType::UNIFORM });

		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_MAT_PARAM_BUFFER",		EExternalInputAttachmentType::BUFFER });
		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_VERTEX_BUFFER",			EExternalInputAttachmentType::BUFFER });
		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_INDEX_BUFFER",			EExternalInputAttachmentType::BUFFER });
		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_INSTANCE_BUFFER",			EExternalInputAttachmentType::BUFFER });
		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_MESH_INDEX_BUFFER",		EExternalInputAttachmentType::BUFFER });

		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_ALBEDO_MAPS",				EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });
		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_NORMAL_MAPS",				EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });
		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_AO_MAPS",					EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });
		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_ROUGHNESS_MAPS",			EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });
		geometryRenderStageExternalInputAttachment.push_back({ "SCENE_METALLIC_MAPS",			EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });

		geometryRenderStageOutputAttachment.push_back({ "GBUFFER_ALBEDO_AO",					EOutputAttachmentType::TEXTURE });
		geometryRenderStageOutputAttachment.push_back({ "GBUFFER_NORMAL_ROUGHNESS_METALLIC",	EOutputAttachmentType::TEXTURE });
		geometryRenderStageOutputAttachment.push_back({ "GBUFFER_VELOCITY",						EOutputAttachmentType::TEXTURE });
		geometryRenderStageOutputAttachment.push_back({ "GBUFFER_DEPTH",						EOutputAttachmentType::DEPTH_STENCIL });

		RenderStage renderStage = {};
		renderStage.pName						= "Geometry Render Stage";
		renderStage.pInputAttachments			= geometryRenderStageInputAttachment.data();
		renderStage.InputAttachmentCount		= geometryRenderStageInputAttachment.size();
		renderStage.pExternalInputAttachments	= geometryRenderStageExternalInputAttachment.data();
		renderStage.ExtenalInputAttachmentCount = geometryRenderStageExternalInputAttachment.size();
		renderStage.pOutputAttachments			= geometryRenderStageOutputAttachment.data();
		renderStage.OutputAttachmentCount		= geometryRenderStageOutputAttachment.size();

		renderStage.PipelineType = EPipelineStateType::GRAPHICS;

		renderStages.push_back(renderStage);
	}

	std::vector<RenderStageInputAttachment>			rayTraceRenderStageInputAttachment;
	std::vector<RenderStageExternalInputAttachment> rayTraceRenderStageExternalInputAttachment;
	std::vector<RenderStageOutputAttachment>		rayTraceRenderStageOutputAttachment;

	{
		rayTraceRenderStageInputAttachment.push_back({ "RADIANCE_IMAGE",					EInputAttachmentType::TEXTURE });
		rayTraceRenderStageInputAttachment.push_back({ "GBUFFER_NORMAL_ROUGHNESS_METALLIC",	EInputAttachmentType::TEXTURE });
		rayTraceRenderStageInputAttachment.push_back({ "GBUFFER_VELOCITY",					EInputAttachmentType::TEXTURE });
		rayTraceRenderStageInputAttachment.push_back({ "GBUFFER_DEPTH",						EInputAttachmentType::TEXTURE });

		rayTraceRenderStageExternalInputAttachment.push_back({ "PER_FRAME_BUFFER",			EExternalInputAttachmentType::UNIFORM });
		rayTraceRenderStageExternalInputAttachment.push_back({ "LIGHTS_BUFFER",				EExternalInputAttachmentType::UNIFORM });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_TLAS",				EExternalInputAttachmentType::UNIFORM });

		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_MAT_PARAM_BUFFER",	EExternalInputAttachmentType::BUFFER });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_VERTEX_BUFFER",		EExternalInputAttachmentType::BUFFER });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_INDEX_BUFFER",		EExternalInputAttachmentType::BUFFER });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_INSTANCE_BUFFER",		EExternalInputAttachmentType::BUFFER });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_MESH_INDEX_BUFFER",	EExternalInputAttachmentType::BUFFER });

		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_ALBEDO_MAPS",			EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_NORMAL_MAPS",			EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_AO_MAPS",				EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_ROUGHNESS_MAPS",		EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });
		rayTraceRenderStageExternalInputAttachment.push_back({ "SCENE_METALLIC_MAPS",		EExternalInputAttachmentType::TEXTURE, MAX_UNIQUE_MATERIALS });

		rayTraceRenderStageExternalInputAttachment.push_back({ "BRDF_LUT",					EExternalInputAttachmentType::TEXTURE });
		rayTraceRenderStageExternalInputAttachment.push_back({ "BLUE_NOISE_LUT",			EExternalInputAttachmentType::TEXTURE });

		rayTraceRenderStageOutputAttachment.push_back({ "RADIANCE_IMAGE",					EOutputAttachmentType::TEXTURE });

		RenderStage renderStage = {};
		renderStage.pName						= "Ray Tracing Render Stage";
		renderStage.pInputAttachments			= rayTraceRenderStageInputAttachment.data();
		renderStage.InputAttachmentCount		= rayTraceRenderStageInputAttachment.size();
		renderStage.pExternalInputAttachments	= rayTraceRenderStageExternalInputAttachment.data();
		renderStage.ExtenalInputAttachmentCount = rayTraceRenderStageExternalInputAttachment.size();
		renderStage.pOutputAttachments			= rayTraceRenderStageOutputAttachment.data();
		renderStage.OutputAttachmentCount		= rayTraceRenderStageOutputAttachment.size();

		renderStage.PipelineType = EPipelineStateType::RAY_TRACING;

		renderStages.push_back(renderStage);
	}

	std::vector<RenderStageInputAttachment>			spatialBlurRenderStageInputAttachment;
	std::vector<RenderStageExternalInputAttachment> spatialBlurRenderStageExternalInputAttachment;
	std::vector<RenderStageOutputAttachment>		spatialBlurRenderStageOutputAttachment;

	{
		spatialBlurRenderStageInputAttachment.push_back({ "RADIANCE_IMAGE",					EInputAttachmentType::TEXTURE });

		spatialBlurRenderStageOutputAttachment.push_back({ "FILTERED_RADIANCE_IMAGE",					EOutputAttachmentType::TEXTURE });

		RenderStage renderStage = {};
		renderStage.pName						= "Spatial Blur Render Stage";
		renderStage.pInputAttachments			= spatialBlurRenderStageInputAttachment.data();
		renderStage.InputAttachmentCount		= spatialBlurRenderStageInputAttachment.size();
		renderStage.pExternalInputAttachments	= spatialBlurRenderStageExternalInputAttachment.data();
		renderStage.ExtenalInputAttachmentCount = spatialBlurRenderStageExternalInputAttachment.size();
		renderStage.pOutputAttachments			= spatialBlurRenderStageOutputAttachment.data();
		renderStage.OutputAttachmentCount		= spatialBlurRenderStageOutputAttachment.size();

		renderStage.PipelineType = EPipelineStateType::COMPUTE;

		renderStages.push_back(renderStage);
	}

	std::vector<RenderStageInputAttachment>			particleUpdateRenderStageInputAttachment;
	std::vector<RenderStageExternalInputAttachment> particleUpdateRenderStageExternalInputAttachment;
	std::vector<RenderStageOutputAttachment>		particleUpdateRenderStageOutputAttachment;

	{
		particleUpdateRenderStageInputAttachment.push_back({ "PARTICLE_BUFFER",					EInputAttachmentType::BUFFER });

		particleUpdateRenderStageOutputAttachment.push_back({ "PARTICLE_BUFFER",					EOutputAttachmentType::BUFFER });

		RenderStage renderStage = {};
		renderStage.pName						= "Particle Update Render Stage";
		renderStage.pInputAttachments			= particleUpdateRenderStageInputAttachment.data();
		renderStage.InputAttachmentCount		= particleUpdateRenderStageInputAttachment.size();
		renderStage.pExternalInputAttachments	= particleUpdateRenderStageExternalInputAttachment.data();
		renderStage.ExtenalInputAttachmentCount = particleUpdateRenderStageExternalInputAttachment.size();
		renderStage.pOutputAttachments			= particleUpdateRenderStageOutputAttachment.data();
		renderStage.OutputAttachmentCount		= particleUpdateRenderStageOutputAttachment.size();

		renderStage.PipelineType = EPipelineStateType::COMPUTE;

		renderStages.push_back(renderStage);
	}

	std::vector<RenderStageInputAttachment>			shadingRenderStageInputAttachment;
	std::vector<RenderStageExternalInputAttachment> shadingRenderStageExternalInputAttachment;
	std::vector<RenderStageOutputAttachment>		shadingRenderStageOutputAttachment;

	{
		shadingRenderStageInputAttachment.push_back({ "FILTERED_RADIANCE_IMAGE",				EInputAttachmentType::TEXTURE });
		shadingRenderStageInputAttachment.push_back({ "GBUFFER_ALBEDO_AO",						EInputAttachmentType::TEXTURE });
		shadingRenderStageInputAttachment.push_back({ "GBUFFER_NORMAL_ROUGHNESS_METALLIC",		EInputAttachmentType::TEXTURE });
		shadingRenderStageInputAttachment.push_back({ "GBUFFER_VELOCITY",						EInputAttachmentType::TEXTURE });
		shadingRenderStageInputAttachment.push_back({ "GBUFFER_DEPTH",							EInputAttachmentType::TEXTURE });

		shadingRenderStageExternalInputAttachment.push_back({ "PER_FRAME_BUFFER",				EExternalInputAttachmentType::UNIFORM });
		shadingRenderStageExternalInputAttachment.push_back({ "LIGHTS_BUFFER",					EExternalInputAttachmentType::UNIFORM });

		shadingRenderStageExternalInputAttachment.push_back({ "BRDF_LUT",						EExternalInputAttachmentType::TEXTURE });
		shadingRenderStageExternalInputAttachment.push_back({ "BLUE_NOISE_LUT",					EExternalInputAttachmentType::TEXTURE });

		shadingRenderStageOutputAttachment.push_back({ RENDER_GRAPH_BACK_BUFFER,				EOutputAttachmentType::TEXTURE });

		RenderStage renderStage = {};
		renderStage.pName						= "Shading Render Stage";
		renderStage.pInputAttachments			= shadingRenderStageInputAttachment.data();
		renderStage.InputAttachmentCount		= shadingRenderStageInputAttachment.size();
		renderStage.pExternalInputAttachments	= shadingRenderStageExternalInputAttachment.data();
		renderStage.ExtenalInputAttachmentCount = shadingRenderStageExternalInputAttachment.size();
		renderStage.pOutputAttachments			= shadingRenderStageOutputAttachment.data();
		renderStage.OutputAttachmentCount		= shadingRenderStageOutputAttachment.size();

		renderStage.PipelineType = EPipelineStateType::GRAPHICS;

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

	RenderGraphCreator::Create(renderGraphDesc);

	clock.Tick();
	LOG_INFO("Render Graph Build Time: %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

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
	LOG_MESSAGE("Mouse Scrolled: %d", delta);
}

void Sandbox::Tick(LambdaEngine::Timestamp dt)
{
	using namespace LambdaEngine;

	m_Timer += dt.AsSeconds();

	if (m_pGunSoundEffect != nullptr)
	{
		if (m_SpawnPlayAts)
		{
			m_GunshotTimer += dt.AsSeconds();

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
