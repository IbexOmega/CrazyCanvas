#include "Sandbox.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderGraphDescriptionParser.h"
#include "Rendering/Core/API/ITextureView.h"
#include "Rendering/Core/API/ISampler.h"

#include "Audio/AudioSystem.h"
#include "Audio/API/IAudioListener.h"
#include "Audio/API/ISoundEffect3D.h"
#include "Audio/API/ISoundInstance3D.h"
#include "Audio/API/IAudioGeometry.h"
#include "Audio/API/IReverbSphere.h"

#include "Game/Scene.h"

#include "Time/API/Clock.h"

#include "Threading/API/Thread.h"

Sandbox::Sandbox()
    : Game()
{
	using namespace LambdaEngine;

	constexpr const uint32 BACK_BUFFER_COUNT = 3;
	constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 5;

	m_pResourceManager = DBG_NEW LambdaEngine::ResourceManager(LambdaEngine::RenderSystem::GetDevice(), LambdaEngine::AudioSystem::GetDevice());
	m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice(), m_pResourceManager);

	SceneDesc sceneDesc = {};
	sceneDesc.pName = "Test Scene";
	m_pScene->Init(sceneDesc);

	std::vector<GameObject>	sceneGameObjects;
	m_pResourceManager->LoadSceneFromFile("../Assets/Scenes/sponza/", "sponza.obj", sceneGameObjects);

	for (GameObject& gameObject : sceneGameObjects)
	{
		m_pScene->AddDynamicGameObject(gameObject, glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)));
	}

	uint32 bunnyMeshGUID = m_pResourceManager->LoadMeshFromFile("../Assets/Meshes/bunny.obj");

	GameObject bunnyGameObject = {};
	bunnyGameObject.Mesh = bunnyMeshGUID;
	bunnyGameObject.Material = DEFAULT_MATERIAL;

	m_pScene->AddDynamicGameObject(bunnyGameObject, glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)));

	uint32 gunMeshGUID = m_pResourceManager->LoadMeshFromFile("../Assets/Meshes/gun.obj");

	GameObject gunGameObject = {};
	gunGameObject.Mesh = gunMeshGUID;
	gunGameObject.Material = DEFAULT_MATERIAL;

	m_pScene->AddDynamicGameObject(gunGameObject, glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
	m_pScene->AddDynamicGameObject(gunGameObject, glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.5f, 0.0f)));

	m_pScene->Finalize();

	m_pCamera = DBG_NEW Camera();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= 90.0f;
	cameraDesc.Width		= 1440.0f;
	cameraDesc.Height		= 900.0f;
	cameraDesc.NearPlane	= 0.001f;
	cameraDesc.FarPlane		= 1000.0f;

	m_pCamera->Init(cameraDesc);

	//GUID_Lambda blurShaderGUID					= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/blur.spv",					FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,		EShaderLang::SPIRV);
	
	//GUID_Lambda lightVertexShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/lightVertex.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	//GUID_Lambda lightPixelShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/lightPixel.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);

	//GUID_Lambda raygenRadianceShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/raygenRadiance.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			EShaderLang::SPIRV);
	//GUID_Lambda closestHitRadianceShaderGUID	= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/closestHitRadiance.spv",	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::SPIRV);
	//GUID_Lambda missRadianceShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/missRadiance.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::SPIRV);
	//GUID_Lambda closestHitShadowShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/closestHitShadow.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::SPIRV);
	//GUID_Lambda missShadowShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/missShadow.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::SPIRV);

	//GUID_Lambda particleUpdateShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/particleUpdate.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,		EShaderLang::SPIRV);

	SamplerDesc samplerLinearDesc = {};
	samplerLinearDesc.pName					= "Linear Sampler";
	samplerLinearDesc.MinFilter				= EFilter::LINEAR;
	samplerLinearDesc.MagFilter				= EFilter::LINEAR;
	samplerLinearDesc.MipmapMode			= EMipmapMode::LINEAR;
	samplerLinearDesc.AddressModeU			= EAddressMode::REPEAT;
	samplerLinearDesc.AddressModeV			= EAddressMode::REPEAT;
	samplerLinearDesc.AddressModeW			= EAddressMode::REPEAT;
	samplerLinearDesc.MipLODBias			= 0.0f;
	samplerLinearDesc.AnisotropyEnabled		= false;
	samplerLinearDesc.MaxAnisotropy			= 16;
	samplerLinearDesc.MinLOD				= 0.0f;
	samplerLinearDesc.MaxLOD				= 1.0f;

	m_pLinearSampler = RenderSystem::GetDevice()->CreateSampler(samplerLinearDesc);

	SamplerDesc samplerNearestDesc = {};
	samplerNearestDesc.pName				= "Nearest Sampler";
	samplerNearestDesc.MinFilter			= EFilter::NEAREST;
	samplerNearestDesc.MagFilter			= EFilter::NEAREST;
	samplerNearestDesc.MipmapMode			= EMipmapMode::NEAREST;
	samplerNearestDesc.AddressModeU			= EAddressMode::REPEAT;
	samplerNearestDesc.AddressModeV			= EAddressMode::REPEAT;
	samplerNearestDesc.AddressModeW			= EAddressMode::REPEAT;
	samplerNearestDesc.MipLODBias			= 0.0f;
	samplerNearestDesc.AnisotropyEnabled	= false;
	samplerNearestDesc.MaxAnisotropy		= 16;
	samplerNearestDesc.MinLOD				= 0.0f;
	samplerNearestDesc.MaxLOD				= 1.0f;

	m_pNearestSampler = RenderSystem::GetDevice()->CreateSampler(samplerNearestDesc);

	InitRendererForDeferred(BACK_BUFFER_COUNT, MAX_TEXTURES_PER_DESCRIPTOR_SET);

	//InitRendererForVisBuf(BACK_BUFFER_COUNT, MAX_TEXTURES_PER_DESCRIPTOR_SET);

	//InitTestAudio();
}

Sandbox::~Sandbox()
{
	SAFEDELETE(m_pResourceManager);
	SAFEDELETE(m_pAudioListener);
	SAFEDELETE(m_pAudioGeometry);

	SAFEDELETE(m_pScene);
	SAFEDELETE(m_pCamera);
	SAFERELEASE(m_pLinearSampler);
	SAFERELEASE(m_pNearestSampler);

	SAFEDELETE(m_pRenderGraph);
	SAFEDELETE(m_pRenderer);
}

void Sandbox::InitTestAudio()
{
	using namespace LambdaEngine;

	m_ToneSoundEffectGUID = m_pResourceManager->LoadSoundEffectFromFile("../Assets/Sounds/noise.wav");
	m_GunSoundEffectGUID = m_pResourceManager->LoadSoundEffectFromFile("../Assets/Sounds/GUN_FIRE-GoodSoundForYou.wav");

	m_pToneSoundEffect = m_pResourceManager->GetSoundEffect(m_ToneSoundEffectGUID);
	m_pGunSoundEffect = m_pResourceManager->GetSoundEffect(m_GunSoundEffectGUID);

	SoundInstance3DDesc soundInstanceDesc = {};
	soundInstanceDesc.pSoundEffect = m_pToneSoundEffect;
	soundInstanceDesc.Flags = FSoundModeFlags::SOUND_MODE_LOOPING;

	m_pToneSoundInstance = AudioSystem::GetDevice()->CreateSoundInstance();
	m_pToneSoundInstance->Init(soundInstanceDesc);
	m_pToneSoundInstance->SetVolume(0.5f);

	/*m_SpawnPlayAts = false;
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

	m_pAudioGeometry->Init(audioGeometryDesc);*/

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

	if (key == EKey::KEY_A)
	{
		LOG_MESSAGE("A Key Pressed");
	}

	static bool geometryAudioActive = true;
	static bool reverbSphereActive = true;

	if (key == EKey::KEY_KEYPAD_1)
	{
		m_pToneSoundInstance->Toggle();
	}
	else if (key == EKey::KEY_KEYPAD_2)
	{
		m_SpawnPlayAts = !m_SpawnPlayAts;
	}
	else if (key == EKey::KEY_KEYPAD_3)
	{
		m_pGunSoundEffect->PlayOnceAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f), 1.0f);
	}
	else if (key == EKey::KEY_KEYPAD_ADD)
	{
		m_GunshotDelay += 0.05f;
	}
	else if (key == EKey::KEY_KEYPAD_SUBTRACT)
	{
		m_GunshotDelay = glm::max(m_GunshotDelay - 0.05f, 0.125f);
	}
	else if (key == EKey::KEY_KEYPAD_5)
	{
		AudioSystem::GetDevice()->ToggleMusic();
	}
	else if (key == EKey::KEY_KEYPAD_7)
	{
		if (m_pAudioGeometry != nullptr)
		{
			geometryAudioActive = !geometryAudioActive;
			LOG_MESSAGE("AudioGeometry %s", geometryAudioActive ? "Enabled" : "Disabled");
			m_pAudioGeometry->SetActive(geometryAudioActive);
		}
	}
	else if (key == EKey::KEY_KEYPAD_8)
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
	UNREFERENCED_VARIABLE(key);
	//LOG_MESSAGE("Key Held Down: %d", key);
}

void Sandbox::OnKeyUp(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
	//LOG_MESSAGE("Key Released: %d", key);
}

void Sandbox::OnMouseMove(int32 x, int32 y)
{
	UNREFERENCED_VARIABLE(x);
	UNREFERENCED_VARIABLE(y);
	//LOG_MESSAGE("Mouse Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button)
{
	UNREFERENCED_VARIABLE(button);
	LOG_MESSAGE("Mouse Button Pressed: %d", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	UNREFERENCED_VARIABLE(button);
	LOG_MESSAGE("Mouse Button Released: %d", button);
}

void Sandbox::OnScroll(int32 delta)
{
	UNREFERENCED_VARIABLE(delta);
	//LOG_MESSAGE("Mouse Scrolled: %d", delta);
}


void Sandbox::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

    //LOG_MESSAGE("Delta: %.6f ms", delta.AsMilliSeconds());
    
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

	if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_W) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_S))
	{
		m_pCamera->Translate(glm::vec3(0.0f, 0.0f, CAMERA_MOVEMENT_SPEED * delta.AsSeconds()));
	}
	else if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_S) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_W))
	{
		m_pCamera->Translate(glm::vec3(0.0f, 0.0f, -CAMERA_MOVEMENT_SPEED * delta.AsSeconds()));
	}

	if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_A) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_D))
	{
		m_pCamera->Translate(glm::vec3(-CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}
	else if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_D) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_A))
	{
		m_pCamera->Translate(glm::vec3(CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}

	if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_Q) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_E))
	{
		m_pCamera->Translate(glm::vec3(0.0f, CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f));
	}
	else if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_E) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_Q))
	{
		m_pCamera->Translate(glm::vec3(0.0f, -CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f));
	}

	if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_UP) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_DOWN))
	{
		m_pCamera->Rotate(glm::vec3(-CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}
	else if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_DOWN) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_UP))
	{
		m_pCamera->Rotate(glm::vec3(CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
	}
	
	if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_LEFT) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_RIGHT))
	{
		m_pCamera->Rotate(glm::vec3(0.0f, -CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f));
	}
	else if (Input::GetKeyboardState().IsKeyDown(EKey::KEY_RIGHT) && Input::GetKeyboardState().IsKeyUp(EKey::KEY_LEFT))
	{
		m_pCamera->Rotate(glm::vec3(0.0f, CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f));
	}

	m_pCamera->Update();
	m_pScene->UpdateCamera(m_pCamera);
		
	m_pRenderer->Render();
}

void Sandbox::FixedTick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
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

bool Sandbox::InitRendererForDeferred(uint32 backBufferCount, uint32 maxTexturesPerDescriptorSet)
{
	using namespace LambdaEngine;

	GUID_Lambda geometryVertexShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/geometryDefVertex.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	GUID_Lambda geometryPixelShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/geometryDefPixel.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);

	GUID_Lambda fullscreenQuadShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/fullscreenQuad.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	GUID_Lambda shadingPixelShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/shadingDefPixel.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);

	std::vector<RenderStageDesc> renderStages;

	const char*									pGeometryRenderStageName = "Geometry Render Stage";
	GraphicsPipelineStateDesc					geometryPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			geometryRenderStageAttachments;

	{
		geometryRenderStageAttachments.push_back({ SCENE_VERTEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ SCENE_INDEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ SCENE_INSTANCE_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ SCENE_MESH_INDEX_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});

		geometryRenderStageAttachments.push_back({ PER_FRAME_BUFFER,							EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});

		geometryRenderStageAttachments.push_back({ SCENE_MAT_PARAM_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1 });
		geometryRenderStageAttachments.push_back({ SCENE_ALBEDO_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS });
		geometryRenderStageAttachments.push_back({ SCENE_NORMAL_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS });
		geometryRenderStageAttachments.push_back({ SCENE_AO_MAPS,								EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS });
		geometryRenderStageAttachments.push_back({ SCENE_ROUGHNESS_MAPS,						EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS });
		geometryRenderStageAttachments.push_back({ SCENE_METALLIC_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS });


		geometryRenderStageAttachments.push_back({ "GEOMETRY_ALBEDO_AO_BUFFER",					EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });
		geometryRenderStageAttachments.push_back({ "GEOMETRY_NORM_MET_ROUGH_BUFFER",			EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });
		geometryRenderStageAttachments.push_back({ "GEOMETRY_DEPTH_STENCIL",					EAttachmentType::OUTPUT_DEPTH_STENCIL,								FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName			= "Geometry Pass Push Constants";
		pushConstants.DataSize		= sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName						= pGeometryRenderStageName;
		renderStage.pAttachments				= geometryRenderStageAttachments.data();
		renderStage.AttachmentCount				= (uint32)geometryRenderStageAttachments.size();
		//renderStage.PushConstants				= pushConstants;

		geometryPipelineStateDesc.pName				= "Geometry Pass Pipeline State";
		geometryPipelineStateDesc.pVertexShader		= m_pResourceManager->GetShader(geometryVertexShaderGUID);
		geometryPipelineStateDesc.pPixelShader		= m_pResourceManager->GetShader(geometryPixelShaderGUID);

		renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

		renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::SCENE_INDIRECT;
		renderStage.GraphicsPipeline.pIndexBufferName		= SCENE_INDEX_BUFFER;
		renderStage.GraphicsPipeline.pMeshIndexBufferName	= SCENE_MESH_INDEX_BUFFER;
		renderStage.GraphicsPipeline.pGraphicsDesc			= &geometryPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	const char*									pShadingRenderStageName = "Shading Render Stage";
	GraphicsPipelineStateDesc					shadingPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			shadingRenderStageAttachments;

	{
		shadingRenderStageAttachments.push_back({ "GEOMETRY_ALBEDO_AO_BUFFER",					EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });
		shadingRenderStageAttachments.push_back({ "GEOMETRY_NORM_MET_ROUGH_BUFFER",				EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });
		shadingRenderStageAttachments.push_back({ "GEOMETRY_DEPTH_STENCIL",						EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });

		shadingRenderStageAttachments.push_back({ RENDER_GRAPH_BACK_BUFFER_ATTACHMENT,			EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName			= "Shading Pass Push Constants";
		pushConstants.DataSize		= sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName						= pShadingRenderStageName;
		renderStage.pAttachments				= shadingRenderStageAttachments.data();
		renderStage.AttachmentCount				= (uint32)shadingRenderStageAttachments.size();
		//renderStage.PushConstants				= pushConstants;

		shadingPipelineStateDesc.pName				= "Shading Pass Pipeline State";
		shadingPipelineStateDesc.pVertexShader		= m_pResourceManager->GetShader(fullscreenQuadShaderGUID);
		shadingPipelineStateDesc.pPixelShader		= m_pResourceManager->GetShader(shadingPixelShaderGUID);

		renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

		renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::FULLSCREEN_QUAD;
		renderStage.GraphicsPipeline.pIndexBufferName		= nullptr;
		renderStage.GraphicsPipeline.pMeshIndexBufferName	= nullptr;
		renderStage.GraphicsPipeline.pGraphicsDesc			= &shadingPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	RenderGraphDesc renderGraphDesc = {};
	renderGraphDesc.pName						= "Render Graph";
	renderGraphDesc.CreateDebugGraph			= true;
	renderGraphDesc.pRenderStages				= renderStages.data();
	renderGraphDesc.RenderStageCount			= (uint32)renderStages.size();
	renderGraphDesc.BackBufferCount				= backBufferCount;
	renderGraphDesc.MaxTexturesPerDescriptorSet = maxTexturesPerDescriptorSet;
	renderGraphDesc.pScene						= m_pScene;

	LambdaEngine::Clock clock;
	clock.Reset();
	clock.Tick();

	m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());

	m_pRenderGraph->Init(renderGraphDesc);

	clock.Tick();
	LOG_INFO("Render Graph Build Time: %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

	uint32 renderWidth	= PlatformApplication::Get()->GetWindow()->GetWidth();
	uint32 renderHeight = PlatformApplication::Get()->GetWindow()->GetHeight();
	
	{
		RenderStageParameters geometryRenderStageParameters = {};
		geometryRenderStageParameters.pRenderStageName	= pGeometryRenderStageName;
		geometryRenderStageParameters.Graphics.Width	= renderWidth;
		geometryRenderStageParameters.Graphics.Height	= renderHeight;

		m_pRenderGraph->UpdateRenderStageParameters(geometryRenderStageParameters);
	}

	{
		RenderStageParameters shadingRenderStageParameters = {};
		shadingRenderStageParameters.pRenderStageName	= pShadingRenderStageName;
		shadingRenderStageParameters.Graphics.Width	= renderWidth;
		shadingRenderStageParameters.Graphics.Height	= renderHeight;

		m_pRenderGraph->UpdateRenderStageParameters(shadingRenderStageParameters);
	}

	{
		IBuffer* pBuffer = m_pScene->GetPerFrameBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= PER_FRAME_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetMaterialProperties();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_MAT_PARAM_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetVertexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_VERTEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetIndexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_INDEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetInstanceBufer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_INSTANCE_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetMeshIndexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_MESH_INDEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		TextureDesc textureDesc	= {};
		textureDesc.pName				= "Albedo-AO G-Buffer Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R8G8B8A8_UNORM;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= PlatformApplication::Get()->GetWindow()->GetWidth();
		textureDesc.Height				= PlatformApplication::Get()->GetWindow()->GetHeight();
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc = { };
		textureViewDesc.pName			= "Albedo-AO G-Buffer Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "Nearest Sampler";
		samplerDesc.MinFilter			= EFilter::NEAREST;
		samplerDesc.MagFilter			= EFilter::NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerDesc.AddressModeU		= EAddressMode::REPEAT;
		samplerDesc.AddressModeV		= EAddressMode::REPEAT;
		samplerDesc.AddressModeW		= EAddressMode::REPEAT;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> textureDescriptions(backBufferCount, &textureDesc);
		std::vector<TextureViewDesc*> textureViewDescriptions(backBufferCount, &textureViewDesc);
		std::vector<SamplerDesc*> samplerDescriptions(backBufferCount, &samplerDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "GEOMETRY_ALBEDO_AO_BUFFER";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= textureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= textureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= samplerDescriptions.data();

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		TextureDesc textureDesc	= {};
		textureDesc.pName				= "Norm-Met-Rough G-Buffer Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_R8G8B8A8_UNORM;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= PlatformApplication::Get()->GetWindow()->GetWidth();
		textureDesc.Height				= PlatformApplication::Get()->GetWindow()->GetHeight();
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureView = { };
		textureView.pName				= "Norm-Met-Rough G-Buffer Texture View";
		textureView.Flags				= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureView.Type				= ETextureViewType::TEXTURE_VIEW_2D;
		textureView.Miplevel			= 0;
		textureView.MiplevelCount		= 1;
		textureView.ArrayIndex			= 0;
		textureView.ArrayCount			= 1;
		textureView.Format				= textureDesc.Format;

		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "Nearest Sampler";
		samplerDesc.MinFilter			= EFilter::NEAREST;
		samplerDesc.MagFilter			= EFilter::NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerDesc.AddressModeU		= EAddressMode::REPEAT;
		samplerDesc.AddressModeV		= EAddressMode::REPEAT;
		samplerDesc.AddressModeW		= EAddressMode::REPEAT;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> textureDescriptions(backBufferCount, &textureDesc);
		std::vector<TextureViewDesc*> textureViewDescriptions(backBufferCount, &textureView);
		std::vector<SamplerDesc*> samplerDescriptions(backBufferCount, &samplerDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "GEOMETRY_NORM_MET_ROUGH_BUFFER";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= textureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= textureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= samplerDescriptions.data();

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		TextureDesc textureDesc = {};
		textureDesc.pName				= "Geometry Pass Depth Stencil Texture";
		textureDesc.Type				= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		textureDesc.Format				= EFormat::FORMAT_D24_UNORM_S8_UINT;
		textureDesc.Flags				= FTextureFlags::TEXTURE_FLAG_DEPTH_STENCIL | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width				= PlatformApplication::Get()->GetWindow()->GetWidth();
		textureDesc.Height				= PlatformApplication::Get()->GetWindow()->GetHeight();
		textureDesc.Depth				= 1;
		textureDesc.SampleCount			= 1;
		textureDesc.Miplevels			= 1;
		textureDesc.ArrayCount			= 1;

		TextureViewDesc textureViewDesc = { };
		textureViewDesc.pName			= "Geometry Pass Depth Stencil Texture View";
		textureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_DEPTH_STENCIL | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.MiplevelCount	= 1;
		textureViewDesc.ArrayIndex		= 0;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Format			= textureDesc.Format;

		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "Nearest Sampler";
		samplerDesc.MinFilter			= EFilter::NEAREST;
		samplerDesc.MagFilter			= EFilter::NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerDesc.AddressModeU		= EAddressMode::REPEAT;
		samplerDesc.AddressModeV		= EAddressMode::REPEAT;
		samplerDesc.AddressModeW		= EAddressMode::REPEAT;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> textureDescriptions(backBufferCount, &textureDesc);
		std::vector<TextureViewDesc*> textureViewDescriptions(backBufferCount, &textureViewDesc);
		std::vector<SamplerDesc*> samplerDescriptions(backBufferCount, &samplerDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "GEOMETRY_DEPTH_STENCIL";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= textureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= textureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= samplerDescriptions.data();

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		ITexture** ppAlbedoMaps						= m_pScene->GetAlbedoMaps();
		ITexture** ppNormalMaps						= m_pScene->GetNormalMaps();
		ITexture** ppAmbientOcclusionMaps			= m_pScene->GetAmbientOcclusionMaps();
		ITexture** ppMetallicMaps					= m_pScene->GetMetallicMaps();
		ITexture** ppRoughnessMaps					= m_pScene->GetRoughnessMaps();

		ITextureView** ppAlbedoMapViews				= m_pScene->GetAlbedoMapViews();
		ITextureView** ppNormalMapViews				= m_pScene->GetNormalMapViews();
		ITextureView** ppAmbientOcclusionMapViews	= m_pScene->GetAmbientOcclusionMapViews();
		ITextureView** ppMetallicMapViews			= m_pScene->GetMetallicMapViews();
		ITextureView** ppRoughnessMapViews			= m_pScene->GetRoughnessMapViews();

		std::vector<ISampler*> samplers(MAX_UNIQUE_MATERIALS, m_pLinearSampler);

		ResourceUpdateDesc albedoMapsUpdateDesc = {};
		albedoMapsUpdateDesc.pResourceName								= SCENE_ALBEDO_MAPS;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppAlbedoMaps;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppAlbedoMapViews;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc normalMapsUpdateDesc = {};
		normalMapsUpdateDesc.pResourceName								= SCENE_NORMAL_MAPS;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppNormalMaps;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppNormalMapViews;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc aoMapsUpdateDesc = {};
		aoMapsUpdateDesc.pResourceName									= SCENE_AO_MAPS;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= ppAmbientOcclusionMaps;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= ppAmbientOcclusionMapViews;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= samplers.data();

		ResourceUpdateDesc metallicMapsUpdateDesc = {};
		metallicMapsUpdateDesc.pResourceName							= SCENE_METALLIC_MAPS;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppMetallicMaps;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppMetallicMapViews;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc roughnessMapsUpdateDesc = {};
		roughnessMapsUpdateDesc.pResourceName							= SCENE_ROUGHNESS_MAPS;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= ppRoughnessMaps;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= ppRoughnessMapViews;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= samplers.data();

		m_pRenderGraph->UpdateResource(albedoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(normalMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(aoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(metallicMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(roughnessMapsUpdateDesc);
	}

	m_pRenderGraph->Update();

	m_pRenderer = DBG_NEW Renderer(RenderSystem::GetDevice());

	RendererDesc rendererDesc = {};
	rendererDesc.pName				= "Renderer";
	rendererDesc.pRenderGraph		= m_pRenderGraph;
	rendererDesc.pWindow			= PlatformApplication::Get()->GetWindow();
	rendererDesc.BackBufferCount	= backBufferCount;
	
	m_pRenderer->Init(rendererDesc);

	return true;
}

bool Sandbox::InitRendererForVisBuf(uint32 backBufferCount, uint32 maxTexturesPerDescriptorSet)
{
	using namespace LambdaEngine;

	GUID_Lambda geometryVertexShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/geometryVisVertex.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	GUID_Lambda geometryPixelShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/geometryVisPixel.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);

	GUID_Lambda fullscreenQuadShaderGUID		= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/fullscreenQuad.spv",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::SPIRV);
	GUID_Lambda shadingPixelShaderGUID			= m_pResourceManager->LoadShaderFromFile("../Assets/Shaders/shadingVisPixel.spv",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::SPIRV);

	std::vector<RenderStageDesc> renderStages;

	const char*									pGeometryRenderStageName = "Geometry Render Stage";
	GraphicsPipelineStateDesc					geometryPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			geometryRenderStageAttachments;

	{
		geometryRenderStageAttachments.push_back({ SCENE_VERTEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});
		geometryRenderStageAttachments.push_back({ SCENE_INSTANCE_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});

		geometryRenderStageAttachments.push_back({ PER_FRAME_BUFFER,							EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, 1});

		geometryRenderStageAttachments.push_back({ "GEOMETRY_VISIBILITY_BUFFER",				EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });
		geometryRenderStageAttachments.push_back({ "GEOMETRY_DEPTH_STENCIL",					EAttachmentType::OUTPUT_DEPTH_STENCIL,								FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1 });

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName			= "Geometry Pass Push Constants";
		pushConstants.DataSize		= sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName						= pGeometryRenderStageName;
		renderStage.pAttachments				= geometryRenderStageAttachments.data();
		renderStage.AttachmentCount				= (uint32)geometryRenderStageAttachments.size();
		//renderStage.PushConstants				= pushConstants;

		geometryPipelineStateDesc.pName				= "Geometry Pass Pipeline State";
		geometryPipelineStateDesc.pVertexShader		= m_pResourceManager->GetShader(geometryVertexShaderGUID);
		geometryPipelineStateDesc.pPixelShader		= m_pResourceManager->GetShader(geometryPixelShaderGUID);

		renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

		renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::SCENE_INDIRECT;
		renderStage.GraphicsPipeline.pIndexBufferName		= SCENE_INDEX_BUFFER;
		renderStage.GraphicsPipeline.pMeshIndexBufferName	= SCENE_MESH_INDEX_BUFFER;
		renderStage.GraphicsPipeline.pGraphicsDesc			= &geometryPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	const char*									pShadingRenderStageName = "Shading Render Stage";
	GraphicsPipelineStateDesc					shadingPipelineStateDesc = {};
	std::vector<RenderStageAttachment>			shadingRenderStageAttachments;

	{
		shadingRenderStageAttachments.push_back({ "GEOMETRY_VISIBILITY_BUFFER",					EAttachmentType::INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });

		shadingRenderStageAttachments.push_back({ SCENE_VERTEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1});
		shadingRenderStageAttachments.push_back({ SCENE_INDEX_BUFFER,							EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1});
		shadingRenderStageAttachments.push_back({ SCENE_INSTANCE_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1});
		shadingRenderStageAttachments.push_back({ SCENE_MESH_INDEX_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1});

		shadingRenderStageAttachments.push_back({ PER_FRAME_BUFFER,								EAttachmentType::EXTERNAL_INPUT_CONSTANT_BUFFER,					FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1});

		shadingRenderStageAttachments.push_back({ SCENE_MAT_PARAM_BUFFER,						EAttachmentType::EXTERNAL_INPUT_UNORDERED_ACCESS_BUFFER,			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	1});
		shadingRenderStageAttachments.push_back({ SCENE_ALBEDO_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS});
		shadingRenderStageAttachments.push_back({ SCENE_NORMAL_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS});
		shadingRenderStageAttachments.push_back({ SCENE_AO_MAPS,								EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS});
		shadingRenderStageAttachments.push_back({ SCENE_ROUGHNESS_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS});
		shadingRenderStageAttachments.push_back({ SCENE_METALLIC_MAPS,							EAttachmentType::EXTERNAL_INPUT_SHADER_RESOURCE_COMBINED_SAMPLER,	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	MAX_UNIQUE_MATERIALS});

		shadingRenderStageAttachments.push_back({ RENDER_GRAPH_BACK_BUFFER_ATTACHMENT,			EAttachmentType::OUTPUT_COLOR,										FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	backBufferCount });

		RenderStagePushConstants pushConstants = {};
		pushConstants.pName			= "Shading Pass Push Constants";
		pushConstants.DataSize		= sizeof(int32) * 2;

		RenderStageDesc renderStage = {};
		renderStage.pName						= pShadingRenderStageName;
		renderStage.pAttachments				= shadingRenderStageAttachments.data();
		renderStage.AttachmentCount				= (uint32)shadingRenderStageAttachments.size();
		//renderStage.PushConstants				= pushConstants;

		shadingPipelineStateDesc.pName				= "Shading Pass Pipeline State";
		shadingPipelineStateDesc.pVertexShader		= m_pResourceManager->GetShader(fullscreenQuadShaderGUID);
		shadingPipelineStateDesc.pPixelShader		= m_pResourceManager->GetShader(shadingPixelShaderGUID);

		renderStage.PipelineType						= EPipelineStateType::GRAPHICS;

		renderStage.GraphicsPipeline.DrawType				= ERenderStageDrawType::FULLSCREEN_QUAD;
		renderStage.GraphicsPipeline.pIndexBufferName		= nullptr;
		renderStage.GraphicsPipeline.pMeshIndexBufferName	= nullptr;
		renderStage.GraphicsPipeline.pGraphicsDesc			= &shadingPipelineStateDesc;

		renderStages.push_back(renderStage);
	}

	RenderGraphDesc renderGraphDesc = {};
	renderGraphDesc.pName						= "Render Graph";
	renderGraphDesc.CreateDebugGraph			= true;
	renderGraphDesc.pRenderStages				= renderStages.data();
	renderGraphDesc.RenderStageCount			= (uint32)renderStages.size();
	renderGraphDesc.BackBufferCount				= backBufferCount;
	renderGraphDesc.MaxTexturesPerDescriptorSet = maxTexturesPerDescriptorSet;
	renderGraphDesc.pScene						= m_pScene;

	LambdaEngine::Clock clock;
	clock.Reset();
	clock.Tick();

	m_pRenderGraph = DBG_NEW RenderGraph(RenderSystem::GetDevice());

	m_pRenderGraph->Init(renderGraphDesc);

	clock.Tick();
	LOG_INFO("Render Graph Build Time: %f milliseconds", clock.GetDeltaTime().AsMilliSeconds());

	uint32 renderWidth	= PlatformApplication::Get()->GetWindow()->GetWidth();
	uint32 renderHeight = PlatformApplication::Get()->GetWindow()->GetHeight();
	
	{
		RenderStageParameters geometryRenderStageParameters = {};
		geometryRenderStageParameters.pRenderStageName	= pGeometryRenderStageName;
		geometryRenderStageParameters.Graphics.Width	= renderWidth;
		geometryRenderStageParameters.Graphics.Height	= renderHeight;

		m_pRenderGraph->UpdateRenderStageParameters(geometryRenderStageParameters);
	}

	{
		RenderStageParameters shadingRenderStageParameters = {};
		shadingRenderStageParameters.pRenderStageName	= pShadingRenderStageName;
		shadingRenderStageParameters.Graphics.Width	= renderWidth;
		shadingRenderStageParameters.Graphics.Height	= renderHeight;

		m_pRenderGraph->UpdateRenderStageParameters(shadingRenderStageParameters);
	}

	{
		IBuffer* pBuffer = m_pScene->GetPerFrameBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= PER_FRAME_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetMaterialProperties();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_MAT_PARAM_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetVertexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_VERTEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetIndexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_INDEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetInstanceBufer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_INSTANCE_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		IBuffer* pBuffer = m_pScene->GetMeshIndexBuffer();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.pResourceName					= SCENE_MESH_INDEX_BUFFER;
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &pBuffer;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		TextureDesc visibilityBufferDesc	= {};
		visibilityBufferDesc.pName			= "Visibility Buffer Texture";
		visibilityBufferDesc.Type			= ETextureType::TEXTURE_2D;
		visibilityBufferDesc.MemoryType		= EMemoryType::MEMORY_GPU;
		visibilityBufferDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		visibilityBufferDesc.Flags			= FTextureFlags::TEXTURE_FLAG_RENDER_TARGET | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		visibilityBufferDesc.Width			= PlatformApplication::Get()->GetWindow()->GetWidth();
		visibilityBufferDesc.Height			= PlatformApplication::Get()->GetWindow()->GetHeight();
		visibilityBufferDesc.Depth			= 1;
		visibilityBufferDesc.SampleCount	= 1;
		visibilityBufferDesc.Miplevels		= 1;
		visibilityBufferDesc.ArrayCount		= 1;

		TextureViewDesc visibilityBufferViewDesc = { };
		visibilityBufferViewDesc.pName			= "Visibility Buffer Texture View";
		visibilityBufferViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		visibilityBufferViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		visibilityBufferViewDesc.Miplevel		= 0;
		visibilityBufferViewDesc.MiplevelCount	= 1;
		visibilityBufferViewDesc.ArrayIndex		= 0;
		visibilityBufferViewDesc.ArrayCount		= 1;
		visibilityBufferViewDesc.Format			= visibilityBufferDesc.Format;

		SamplerDesc samplerNearestDesc = {};
		samplerNearestDesc.pName				= "Nearest Sampler";
		samplerNearestDesc.MinFilter			= EFilter::NEAREST;
		samplerNearestDesc.MagFilter			= EFilter::NEAREST;
		samplerNearestDesc.MipmapMode			= EMipmapMode::NEAREST;
		samplerNearestDesc.AddressModeU			= EAddressMode::REPEAT;
		samplerNearestDesc.AddressModeV			= EAddressMode::REPEAT;
		samplerNearestDesc.AddressModeW			= EAddressMode::REPEAT;
		samplerNearestDesc.MipLODBias			= 0.0f;
		samplerNearestDesc.AnisotropyEnabled	= false;
		samplerNearestDesc.MaxAnisotropy		= 16;
		samplerNearestDesc.MinLOD				= 0.0f;
		samplerNearestDesc.MaxLOD				= 1.0f;

		std::vector<TextureDesc*> visibilityBufferTextureDescriptions(backBufferCount, &visibilityBufferDesc);
		std::vector<TextureViewDesc*> visibilityBufferTextureViewDescriptions(backBufferCount, &visibilityBufferViewDesc);
		std::vector<SamplerDesc*> visibilityBufferSamplerDescriptions(backBufferCount, &samplerNearestDesc);

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "GEOMETRY_VISIBILITY_BUFFER";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= visibilityBufferTextureDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= visibilityBufferTextureViewDescriptions.data();
		resourceUpdateDesc.InternalTextureUpdate.ppSamplerDesc		= visibilityBufferSamplerDescriptions.data();

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		TextureDesc depthStencilDesc = {};
		depthStencilDesc.pName			= "Geometry Pass Depth Stencil Texture";
		depthStencilDesc.Type			= ETextureType::TEXTURE_2D;
		depthStencilDesc.MemoryType		= EMemoryType::MEMORY_GPU;
		depthStencilDesc.Format			= EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.Flags			= TEXTURE_FLAG_DEPTH_STENCIL;
		depthStencilDesc.Width			= PlatformApplication::Get()->GetWindow()->GetWidth();
		depthStencilDesc.Height			= PlatformApplication::Get()->GetWindow()->GetHeight();
		depthStencilDesc.Depth			= 1;
		depthStencilDesc.SampleCount	= 1;
		depthStencilDesc.Miplevels		= 1;
		depthStencilDesc.ArrayCount		= 1;

		TextureViewDesc depthStencilViewDesc = { };
		depthStencilViewDesc.pName			= "Geometry Pass Depth Stencil Texture View";
		depthStencilViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_DEPTH_STENCIL;
		depthStencilViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		depthStencilViewDesc.Miplevel		= 0;
		depthStencilViewDesc.MiplevelCount	= 1;
		depthStencilViewDesc.ArrayIndex		= 0;
		depthStencilViewDesc.ArrayCount		= 1;
		depthStencilViewDesc.Format			= depthStencilDesc.Format;

		TextureDesc*		pDepthStencilDesc		= &depthStencilDesc;
		TextureViewDesc*	pDepthStencilViewDesc	= &depthStencilViewDesc;

		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.pResourceName							= "GEOMETRY_DEPTH_STENCIL";
		resourceUpdateDesc.InternalTextureUpdate.ppTextureDesc		= &pDepthStencilDesc;
		resourceUpdateDesc.InternalTextureUpdate.ppTextureViewDesc	= &pDepthStencilViewDesc;

		m_pRenderGraph->UpdateResource(resourceUpdateDesc);
	}

	{
		ITexture** ppAlbedoMaps						= m_pScene->GetAlbedoMaps();
		ITexture** ppNormalMaps						= m_pScene->GetNormalMaps();
		ITexture** ppAmbientOcclusionMaps			= m_pScene->GetAmbientOcclusionMaps();
		ITexture** ppMetallicMaps					= m_pScene->GetMetallicMaps();
		ITexture** ppRoughnessMaps					= m_pScene->GetRoughnessMaps();

		ITextureView** ppAlbedoMapViews				= m_pScene->GetAlbedoMapViews();
		ITextureView** ppNormalMapViews				= m_pScene->GetNormalMapViews();
		ITextureView** ppAmbientOcclusionMapViews	= m_pScene->GetAmbientOcclusionMapViews();
		ITextureView** ppMetallicMapViews			= m_pScene->GetMetallicMapViews();
		ITextureView** ppRoughnessMapViews			= m_pScene->GetRoughnessMapViews();

		std::vector<ISampler*> samplers(MAX_UNIQUE_MATERIALS, m_pLinearSampler);

		ResourceUpdateDesc albedoMapsUpdateDesc = {};
		albedoMapsUpdateDesc.pResourceName								= SCENE_ALBEDO_MAPS;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppAlbedoMaps;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppAlbedoMapViews;
		albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc normalMapsUpdateDesc = {};
		normalMapsUpdateDesc.pResourceName								= SCENE_NORMAL_MAPS;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppNormalMaps;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppNormalMapViews;
		normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc aoMapsUpdateDesc = {};
		aoMapsUpdateDesc.pResourceName									= SCENE_AO_MAPS;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextures				= ppAmbientOcclusionMaps;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews			= ppAmbientOcclusionMapViews;
		aoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers				= samplers.data();

		ResourceUpdateDesc metallicMapsUpdateDesc = {};
		metallicMapsUpdateDesc.pResourceName							= SCENE_METALLIC_MAPS;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= ppMetallicMaps;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= ppMetallicMapViews;
		metallicMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= samplers.data();

		ResourceUpdateDesc roughnessMapsUpdateDesc = {};
		roughnessMapsUpdateDesc.pResourceName							= SCENE_ROUGHNESS_MAPS;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= ppRoughnessMaps;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= ppRoughnessMapViews;
		roughnessMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= samplers.data();

		m_pRenderGraph->UpdateResource(albedoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(normalMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(aoMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(metallicMapsUpdateDesc);
		m_pRenderGraph->UpdateResource(roughnessMapsUpdateDesc);
	}

	m_pRenderGraph->Update();

	m_pRenderer = DBG_NEW Renderer(RenderSystem::GetDevice());

	RendererDesc rendererDesc = {};
	rendererDesc.pName				= "Renderer";
	rendererDesc.pRenderGraph		= m_pRenderGraph;
	rendererDesc.pWindow			= PlatformApplication::Get()->GetWindow();
	rendererDesc.BackBufferCount	= backBufferCount;
	
	m_pRenderer->Init(rendererDesc);

	return true;
}