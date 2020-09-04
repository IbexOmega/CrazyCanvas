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

#include "Game/Scene.h"

#include "Time/API/Clock.h"

#include "Threading/API/Thread.h"

#include <imgui.h>

constexpr const uint32 BACK_BUFFER_COUNT = 3;
#ifdef LAMBDA_PLATFORM_MACOS
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 8;
#else
constexpr const uint32 MAX_TEXTURES_PER_DESCRIPTOR_SET = 256;
#endif
constexpr const bool SHOW_DEMO					= true;
constexpr const bool RAY_TRACING_ENABLED		= false;
constexpr const bool SVGF_ENABLED				= false;
constexpr const bool POST_PROCESSING_ENABLED	= false;

constexpr const bool RENDER_GRAPH_IMGUI_ENABLED	= true;
constexpr const bool RENDERING_DEBUG_ENABLED	= false;

constexpr const float DEFAULT_DIR_LIGHT_R			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_G			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_B			= 1.0f;
constexpr const float DEFAULT_DIR_LIGHT_STRENGTH	= 0.0f;

constexpr const uint32 NUM_BLUE_NOISE_LUTS = 128;

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

	CommonApplication::Get()->AddEventHandler(this);

	ShaderReflection shaderReflection;
	ResourceLoader::CreateShaderReflection("../Assets/Shaders/Raygen.rgen", FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER, EShaderLang::SHADER_LANG_GLSL, &shaderReflection);
	
	m_pScene = DBG_NEW Scene(RenderSystem::GetDevice(), AudioSystem::GetDevice());

	SceneDesc sceneDesc = {};
	sceneDesc.Name				= "Test Scene";
	sceneDesc.RayTracingEnabled = RAY_TRACING_ENABLED;
	m_pScene->Init(sceneDesc);

	m_DirectionalLightAngle	= glm::half_pi<float>();
	m_DirectionalLightStrength[0] = DEFAULT_DIR_LIGHT_R;
	m_DirectionalLightStrength[1] = DEFAULT_DIR_LIGHT_G;
	m_DirectionalLightStrength[2] = DEFAULT_DIR_LIGHT_B;
	m_DirectionalLightStrength[3] = DEFAULT_DIR_LIGHT_STRENGTH;

	DirectionalLight directionalLight;
	directionalLight.Direction			= glm::vec4(glm::normalize(glm::vec3(glm::cos(m_DirectionalLightAngle), glm::sin(m_DirectionalLightAngle), 0.0f)), 0.0f);
	directionalLight.EmittedRadiance	= glm::vec4(glm::vec3(m_DirectionalLightStrength[0], m_DirectionalLightStrength[1], m_DirectionalLightStrength[2]) * m_DirectionalLightStrength[3], 0.0f);

	EScene scene = EScene::SPONZA;

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

	m_pCamera = DBG_NEW Camera();

	Window* pWindow = CommonApplication::Get()->GetMainWindow();

	CameraDesc cameraDesc = {};
	cameraDesc.FOVDegrees	= 90.0f;
	cameraDesc.Width		= pWindow->GetWidth();
	cameraDesc.Height		= pWindow->GetHeight();
	cameraDesc.NearPlane	= 0.001f;
	cameraDesc.FarPlane		= 1000.0f;

	m_pCamera->Init(cameraDesc);
	m_pCamera->Update();

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

	//InitRendererForEmpty();

	//InitRendererForRayTracingOnly();

	InitRendererForDeferred();

	//InitRendererForVisBuf(BACK_BUFFER_COUNT, MAX_TEXTURES_PER_DESCRIPTOR_SET);

	//CommandList* pGraphicsCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();
	//CommandList* pComputeCopyCommandList = m_pRenderer->AcquireComputeCopyCommandList();

	m_pScene->UpdateCamera(m_pCamera);

	if (RENDER_GRAPH_IMGUI_ENABLED)
	{
		m_pRenderGraphEditor->InitGUI();	//Must Be called after Renderer is initialized
	}

	return;
}

Sandbox::~Sandbox()
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

void Sandbox::InitTestAudio()
{
	using namespace LambdaEngine;

	//m_AudioListenerIndex = AudioSystem::GetDevice()->CreateAudioListener();

	//m_ToneSoundEffectGUID = ResourceManager::LoadSoundEffectFromFile("../Assets/Sounds/noise.wav");
	//m_GunSoundEffectGUID = ResourceManager::LoadSoundEffectFromFile("../Assets/Sounds/GUN_FIRE-GoodSoundForYou.wav");

	//m_pToneSoundEffect = ResourceManager::GetSoundEffect(m_ToneSoundEffectGUID);
	//m_pGunSoundEffect = ResourceManager::GetSoundEffect(m_GunSoundEffectGUID);

	//SoundInstance3DDesc soundInstanceDesc = {};
	//soundInstanceDesc.pSoundEffect = m_pGunSoundEffect;
	//soundInstanceDesc.Flags = FSoundModeFlags::SOUND_MODE_LOOPING;

	//m_pToneSoundInstance = AudioSystem::GetDevice()->CreateSoundInstance(&soundInstanceDesc);
	//m_pToneSoundInstance->SetVolume(0.5f);

	MusicDesc musicDesc = {};
	musicDesc.pFilepath		= "../Assets/Sounds/halo_theme.ogg";
	musicDesc.Volume		= 0.5f;
	musicDesc.Pitch			= 1.0f;

	AudioSystem::GetDevice()->CreateMusic(&musicDesc);

	/*m_SpawnPlayAts = false;
	m_GunshotTimer = 0.0f;
	m_GunshotDelay = 1.0f;
	m_Timer = 0.0f;


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

	GUID_Lambda sphereGUID = ResourceManager::LoadMeshFromFile("../Assets/Meshes/sphere.obj");
	Mesh* sphereMesh = ResourceManager::GetMesh(sphereGUID);
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
	ResourceManager::LoadSceneFromFile("../Assets/Scenes/sponza/", "sponza.obj", sponzaGraphicsObjects);

	std::vector<Mesh*> sponzaMeshes;
	std::vector<glm::mat4> sponzaMeshTransforms;
	std::vector<LambdaEngine::AudioMeshParameters> sponzaAudioMeshParameters;

	for (GraphicsObject& graphicsObject : sponzaGraphicsObjects)
	{
		sponzaMeshes.push_back(ResourceManager::GetMesh(graphicsObject.Mesh));
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

void Sandbox::OnFocusChanged(LambdaEngine::Window* pWindow, bool hasFocus)
{
	UNREFERENCED_VARIABLE(hasFocus);
	UNREFERENCED_VARIABLE(pWindow);
	
	//LOG_MESSAGE("Window Moved: hasFocus=%s", hasFocus ? "true" : "false");
}

void Sandbox::OnWindowMoved(LambdaEngine::Window* pWindow, int16 x, int16 y)
{
	UNREFERENCED_VARIABLE(x);
	UNREFERENCED_VARIABLE(y);
	UNREFERENCED_VARIABLE(pWindow);
	
	//LOG_MESSAGE("Window Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnWindowResized(LambdaEngine::Window* pWindow, uint16 width, uint16 height, LambdaEngine::EResizeType type)
{
	UNREFERENCED_VARIABLE(pWindow);
	UNREFERENCED_VARIABLE(type);
	
	//LOG_MESSAGE("Window Resized: width=%u, height=%u, type=%u", width, height, uint32(type));
}

void Sandbox::OnWindowClosed(LambdaEngine::Window* pWindow)
{
	UNREFERENCED_VARIABLE(pWindow);
	
   // LOG_MESSAGE("Window closed");
}

void Sandbox::OnMouseEntered(LambdaEngine::Window* pWindow)
{
	UNREFERENCED_VARIABLE(pWindow);
	
	//LOG_MESSAGE("Mouse Entered");
}

void Sandbox::OnMouseLeft(LambdaEngine::Window* pWindow)
{
	UNREFERENCED_VARIABLE(pWindow);
	
	//LOG_MESSAGE("Mouse Left");
}

void Sandbox::OnKeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)
{
	UNREFERENCED_VARIABLE(modifierMask);
	
	using namespace LambdaEngine;
	
	//LOG_MESSAGE("Key Pressed: %s, isRepeat=%s", KeyToString(key), isRepeat ? "true" : "false");

	if (isRepeat)
	{
		return;
	}
	
	Window* pMainWindow = CommonApplication::Get()->GetMainWindow();
	if (key == EKey::KEY_ESCAPE)
	{
		pMainWindow->Close();
	}
	if (key == EKey::KEY_1)
	{
		pMainWindow->Minimize();
	}
	if (key == EKey::KEY_2)
	{
		pMainWindow->Maximize();
	}
	if (key == EKey::KEY_3)
	{
		pMainWindow->Restore();
	}
	if (key == EKey::KEY_4)
	{
		pMainWindow->ToggleFullscreen();
	}
	if (key == EKey::KEY_5)
	{
		if (CommonApplication::Get()->GetInputMode(pMainWindow) == EInputMode::INPUT_MODE_STANDARD)
		{
			CommonApplication::Get()->SetInputMode(pMainWindow, EInputMode::INPUT_MODE_RAW);
		}
		else
		{
			CommonApplication::Get()->SetInputMode(pMainWindow, EInputMode::INPUT_MODE_STANDARD);
		}
	}
	if (key == EKey::KEY_6)
	{
		pMainWindow->SetPosition(0, 0);
	}
	
	static bool geometryAudioActive = true;
	static bool reverbSphereActive = true;

	if (key == EKey::KEY_KEYPAD_5)
	{
		RenderSystem::GetGraphicsQueue()->Flush();
		RenderSystem::GetComputeQueue()->Flush();
		ResourceManager::ReloadAllShaders();
		PipelineStateManager::ReloadPipelineStates();
	}
	/*if (key == EKey::KEY_KEYPAD_1)
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
	}*/
}

void Sandbox::OnKeyReleased(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;
	
	UNREFERENCED_VARIABLE(key);
	
	//LOG_MESSAGE("Key Released: %s", KeyToString(key));
}

void Sandbox::OnKeyTyped(uint32 character)
{
	using namespace LambdaEngine;
	
	UNREFERENCED_VARIABLE(character);
	
	//LOG_MESSAGE("Key Text: %c", char(character));
}

void Sandbox::OnMouseMoved(int32 x, int32 y)
{
	UNREFERENCED_VARIABLE(x);
	UNREFERENCED_VARIABLE(y);
	
	//LOG_MESSAGE("Mouse Moved: x=%d, y=%d", x, y);
}

void Sandbox::OnMouseMovedRaw(int32 deltaX, int32 deltaY)
{
	UNREFERENCED_VARIABLE(deltaX);
	UNREFERENCED_VARIABLE(deltaY);
	
	//LOG_MESSAGE("Mouse Delta: x=%d, y=%d", deltaX, deltaY);
}

void Sandbox::OnButtonPressed(LambdaEngine::EMouseButton button, uint32 modifierMask)
{
	UNREFERENCED_VARIABLE(button);
	UNREFERENCED_VARIABLE(modifierMask);
	
	//LOG_MESSAGE("Mouse Button Pressed: %d", button);
}

void Sandbox::OnButtonReleased(LambdaEngine::EMouseButton button)
{
	UNREFERENCED_VARIABLE(button);
	//LOG_MESSAGE("Mouse Button Released: %d", button);
}

void Sandbox::OnMouseScrolled(int32 deltaX, int32 deltaY)
{
	UNREFERENCED_VARIABLE(deltaX);
	UNREFERENCED_VARIABLE(deltaY);
	
	//LOG_MESSAGE("Mouse Scrolled: x=%d, y=%d", deltaX, deltaY);
}


void Sandbox::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	Render(delta);
}

void Sandbox::FixedTick(LambdaEngine::Timestamp delta)
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
	m_pScene->UpdateCamera(m_pCamera);

	AudioListenerDesc listenerDesc = {};
	listenerDesc.Position = m_pCamera->GetPosition();
	listenerDesc.Forward = m_pCamera->GetForwardVec();
	listenerDesc.Up = m_pCamera->GetUpVec();

	AudioSystem::GetDevice()->UpdateAudioListener(m_AudioListenerIndex, &listenerDesc);
}

void Sandbox::Render(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	m_pRenderGraph->Update();

	m_pRenderer->NewFrame(delta);

	CommandList* pGraphicsCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();
	CommandList* pComputeCopyCommandList = m_pRenderer->AcquireGraphicsCopyCommandList();

	Window* pWindow = CommonApplication::Get()->GetMainWindow();
	float32 renderWidth = (float32)pWindow->GetWidth();
	float32 renderHeight = (float32)pWindow->GetHeight();
	float32 renderAspectRatio = renderWidth / renderHeight;

	if (RENDER_GRAPH_IMGUI_ENABLED)
	{
		m_pRenderGraphEditor->RenderGUI();

		ImGui::ShowDemoWindow();

		ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Debugging Window", NULL))
		{
			uint32 modFrameIndex = m_pRenderer->GetModFrameIndex();

			TextureView* const* ppTextureViews = nullptr;
			uint32			textureViewCount = 0;

			static ImGuiTexture albedoTexture = {};
			static ImGuiTexture normalTexture = {};
			static ImGuiTexture emissiveMetallicRoughness = {};
			static ImGuiTexture compactNormalsTexture = {};
			static ImGuiTexture motionTexture = {};
			static ImGuiTexture linearZTexture = {};
			static ImGuiTexture depthStencilTexture = {};
			static ImGuiTexture momentsTexture = {};
			static ImGuiTexture radianceTexture = {};
			static ImGuiTexture compNormalTexture = {};

			float windowWidth = ImGui::GetWindowWidth();

			ImGui::Text("FPS: %f", 1.0f / delta.AsSeconds());
			ImGui::Text("Frametime (ms): %f", delta.AsMilliSeconds());

			if (ImGui::BeginTabBar("Debugging Tabs"))
			{
				if (ImGui::BeginTabItem("Scene"))
				{
					bool dirLightChanged = false;

					if (ImGui::SliderFloat("Dir. Light Angle", &m_DirectionalLightAngle, 0.0f, glm::two_pi<float>()))
					{
						dirLightChanged = true;
					}

					if (ImGui::ColorEdit3("Dir. Light Color", m_DirectionalLightStrength))
					{
						dirLightChanged = true;
					}

					if (ImGui::SliderFloat("Dir. Light Strength", &m_DirectionalLightStrength[3], 0.0f, 10000.0f, "%.3f"))
					{
						dirLightChanged = true;
					}

					if (ImGui::Button("Reset Dir. Light"))
					{
						m_DirectionalLightStrength[0] = DEFAULT_DIR_LIGHT_R;
						m_DirectionalLightStrength[1] = DEFAULT_DIR_LIGHT_G;
						m_DirectionalLightStrength[2] = DEFAULT_DIR_LIGHT_B;
						m_DirectionalLightStrength[3] = DEFAULT_DIR_LIGHT_STRENGTH;

						dirLightChanged = true;
					}

					if (dirLightChanged)
					{
						DirectionalLight directionalLight;
						directionalLight.Direction			= glm::vec4(glm::normalize(glm::vec3(glm::cos(m_DirectionalLightAngle), glm::sin(m_DirectionalLightAngle), 0.0f)), 0.0f);
						directionalLight.EmittedRadiance	= glm::vec4(glm::vec3(m_DirectionalLightStrength[0], m_DirectionalLightStrength[1], m_DirectionalLightStrength[2]) * m_DirectionalLightStrength[3], 0.0f);

						m_pScene->SetDirectionalLight(directionalLight);
					}

					if (ImGui::CollapsingHeader("Game Objects"))
					{
						for (InstanceIndexAndTransform& instanceIndexAndTransform : m_InstanceIndicesAndTransforms)
						{
							if (ImGui::TreeNode(("Game Object" + std::to_string(instanceIndexAndTransform.InstanceIndex)).c_str()))
							{
								bool updated = false;

								if (ImGui::SliderFloat3("Position", glm::value_ptr(instanceIndexAndTransform.Position), -50.0f, 50.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat4("Rotation", glm::value_ptr(instanceIndexAndTransform.Rotation), 0.0f, glm::two_pi<float32>()))
								{
									updated = true;
								}

								if (ImGui::SliderFloat3("Scale", glm::value_ptr(instanceIndexAndTransform.Scale), 0.0f, 10.0f))
								{
									updated = true;
								}

								float lockedScale = instanceIndexAndTransform.Scale.x;
								if (ImGui::SliderFloat("Locked Scale", &lockedScale, 0.0f, 10.0f))
								{
									instanceIndexAndTransform.Scale.x = lockedScale;
									instanceIndexAndTransform.Scale.y = lockedScale;
									instanceIndexAndTransform.Scale.z = lockedScale;
									updated = true;
								}

								if (updated)
								{
									glm::mat4 transform(1.0f);
									transform = glm::translate(transform, instanceIndexAndTransform.Position);
									transform = glm::rotate(transform, instanceIndexAndTransform.Rotation.w, glm::vec3(instanceIndexAndTransform.Rotation));
									transform = glm::scale(transform, instanceIndexAndTransform.Scale);

									m_pScene->UpdateTransform(instanceIndexAndTransform.InstanceIndex, transform);
								}

								ImGui::TreePop();
							}
						}
					}

					if (ImGui::CollapsingHeader("Lights"))
					{
						for (InstanceIndexAndTransform& instanceIndexAndTransform : m_LightInstanceIndicesAndTransforms)
						{
							if (ImGui::TreeNode(("Light" + std::to_string(instanceIndexAndTransform.InstanceIndex)).c_str()))
							{
								bool updated = false;

								if (ImGui::SliderFloat3("Position", glm::value_ptr(instanceIndexAndTransform.Position), -50.0f, 50.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat4("Rotation", glm::value_ptr(instanceIndexAndTransform.Rotation), 0.0f, glm::two_pi<float32>()))
								{
									updated = true;
								}

								if (ImGui::SliderFloat3("Scale", glm::value_ptr(instanceIndexAndTransform.Scale), 0.0f, 10.0f))
								{
									updated = true;
								}

								float lockedScale = instanceIndexAndTransform.Scale.x;
								if (ImGui::SliderFloat("Locked Scale",&lockedScale, 0.0f, 10.0f))
								{
									instanceIndexAndTransform.Scale.x = lockedScale;
									instanceIndexAndTransform.Scale.y = lockedScale;
									instanceIndexAndTransform.Scale.z = lockedScale;
									updated = true;
								}

								if (updated)
								{
									glm::mat4 transform(1.0f);
									transform = glm::translate(transform, instanceIndexAndTransform.Position);
									transform = glm::rotate(transform, instanceIndexAndTransform.Rotation.w, glm::vec3(instanceIndexAndTransform.Rotation));
									transform = glm::scale(transform, instanceIndexAndTransform.Scale);

									m_pScene->UpdateTransform(instanceIndexAndTransform.InstanceIndex, transform);
								}

								ImGui::TreePop();
							}
						}
					}

					if (ImGui::CollapsingHeader("Materials"))
					{
						std::unordered_map<String, GUID_Lambda>& materialNamesMap = ResourceManager::GetMaterialNamesMap();;

						for (auto materialIt = materialNamesMap.begin(); materialIt != materialNamesMap.end(); materialIt++)
						{
							if (ImGui::TreeNode(materialIt->first.c_str()))
							{
								Material* pMaterial = ResourceManager::GetMaterial(materialIt->second);

								bool updated = false;

								if (ImGui::ColorEdit3("Albedo", glm::value_ptr(pMaterial->Properties.Albedo)))
								{
									updated = true;
								}

								if (ImGui::SliderFloat("AO", &pMaterial->Properties.Ambient, 0.0f, 1.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat("Metallic", &pMaterial->Properties.Metallic, 0.0f, 1.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat("Roughness", &pMaterial->Properties.Roughness, 0.0f, 1.0f))
								{
									updated = true;
								}

								if (ImGui::SliderFloat("Emission Strength", &pMaterial->Properties.EmissionStrength, 0.0f, 1000.0f))
								{
									updated = true;
								}

								if (updated)
								{
									m_pScene->UpdateMaterialProperties(materialIt->second);
								}

								ImGui::TreePop();
							}
						}
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Albedo AO"))
				{
					albedoTexture.ResourceName = "G_BUFFER_ALBEDO_AO";
					ImGui::Image(&albedoTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Emission Metallic Roughness"))
				{
					emissiveMetallicRoughness.ResourceName = "G_BUFFER_EMISSION_METALLIC_ROUGHNESS";

					const char* items[] = { "Emission", "Metallic", "Roughness" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						emissiveMetallicRoughness.ReservedIncludeMask = 0x00008420;

						emissiveMetallicRoughness.ChannelMul[0] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[1] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[2] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[3] = 0.0f;

						emissiveMetallicRoughness.ChannelAdd[0] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[1] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[2] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[3] = 1.0f;

						emissiveMetallicRoughness.PixelShaderGUID = m_ImGuiPixelShaderEmissionGUID;
					}
					else if (currentItem == 1)
					{
						emissiveMetallicRoughness.ReservedIncludeMask = 0x00001110;

						emissiveMetallicRoughness.ChannelMul[0] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[1] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[2] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[3] = 0.0f;

						emissiveMetallicRoughness.ChannelAdd[0] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[1] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[2] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[3] = 1.0f;

						emissiveMetallicRoughness.PixelShaderGUID = m_ImGuiPixelPackedMetallicGUID;
					}
					else if (currentItem == 2)
					{
						emissiveMetallicRoughness.ReservedIncludeMask = 0x00001110;

						emissiveMetallicRoughness.ChannelMul[0] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[1] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[2] = 1.0f;
						emissiveMetallicRoughness.ChannelMul[3] = 0.0f;

						emissiveMetallicRoughness.ChannelAdd[0] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[1] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[2] = 0.0f;
						emissiveMetallicRoughness.ChannelAdd[3] = 1.0f;

						emissiveMetallicRoughness.PixelShaderGUID = m_ImGuiPixelPackedRoughnessGUID;
					}

					ImGui::Image(&emissiveMetallicRoughness, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Compact Normals"))
				{
					compactNormalsTexture.ResourceName = "G_BUFFER_COMPACT_NORMALS";

					const char* items[] = { "Shading", "Geometric" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						compactNormalsTexture.ReservedIncludeMask = 0x00008880;

						compactNormalsTexture.ChannelMul[0] = 1.0f;
						compactNormalsTexture.ChannelMul[1] = 1.0f;
						compactNormalsTexture.ChannelMul[2] = 1.0f;
						compactNormalsTexture.ChannelMul[3] = 0.0f;

						compactNormalsTexture.ChannelAdd[0] = 0.0f;
						compactNormalsTexture.ChannelAdd[1] = 0.0f;
						compactNormalsTexture.ChannelAdd[2] = 0.0f;
						compactNormalsTexture.ChannelAdd[3] = 1.0f;

						compactNormalsTexture.PixelShaderGUID = m_ImGuiPixelCompactNormalFloatGUID;
					}
					else if (currentItem == 1)
					{
						compactNormalsTexture.ReservedIncludeMask = 0x00004440;

						compactNormalsTexture.ChannelMul[0] = 1.0f;
						compactNormalsTexture.ChannelMul[1] = 1.0f;
						compactNormalsTexture.ChannelMul[2] = 1.0f;
						compactNormalsTexture.ChannelMul[3] = 0.0f;

						compactNormalsTexture.ChannelAdd[0] = 0.0f;
						compactNormalsTexture.ChannelAdd[1] = 0.0f;
						compactNormalsTexture.ChannelAdd[2] = 0.0f;
						compactNormalsTexture.ChannelAdd[3] = 1.0f;

						compactNormalsTexture.PixelShaderGUID = m_ImGuiPixelCompactNormalFloatGUID;
					}

					ImGui::Image(&compactNormalsTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Motion"))
				{
					motionTexture.ResourceName = "G_BUFFER_MOTION";

					motionTexture.ReservedIncludeMask = 0x00008400;

					motionTexture.ChannelMul[0] = 10.0f;
					motionTexture.ChannelMul[1] = 10.0f;
					motionTexture.ChannelMul[2] = 0.0f;
					motionTexture.ChannelMul[3] = 0.0f;

					motionTexture.ChannelAdd[0] = 0.0f;
					motionTexture.ChannelAdd[1] = 0.0f;
					motionTexture.ChannelAdd[2] = 0.0f;
					motionTexture.ChannelAdd[3] = 1.0f;

					ImGui::Image(&motionTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Linear Z"))
				{
					linearZTexture.ResourceName = "G_BUFFER_LINEAR_Z";

					const char* items[] = { "ALL", "Linear Z", "Max Change Z", "Prev Linear Z", "Packed Local Normal" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						linearZTexture.ReservedIncludeMask = 0x00008421;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 1.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 0.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelLinearZGUID;
					}
					else if (currentItem == 1)
					{
						linearZTexture.ReservedIncludeMask = 0x00008880;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 0.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 1.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelLinearZGUID;
					}
					else if (currentItem == 2)
					{
						linearZTexture.ReservedIncludeMask = 0x00004440;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 0.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 1.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelLinearZGUID;
					}
					else if (currentItem == 3)
					{
						linearZTexture.ReservedIncludeMask = 0x00002220;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 0.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 1.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelLinearZGUID;
					}
					else if (currentItem == 4)
					{
						linearZTexture.ReservedIncludeMask = 0x00001110;

						linearZTexture.ChannelMul[0] = 1.0f;
						linearZTexture.ChannelMul[1] = 1.0f;
						linearZTexture.ChannelMul[2] = 1.0f;
						linearZTexture.ChannelMul[3] = 0.0f;

						linearZTexture.ChannelAdd[0] = 0.0f;
						linearZTexture.ChannelAdd[1] = 0.0f;
						linearZTexture.ChannelAdd[2] = 0.0f;
						linearZTexture.ChannelAdd[3] = 1.0f;

						linearZTexture.PixelShaderGUID = m_ImGuiPixelShaderPackedLocalNormalGUID;
					}

					ImGui::Image(&linearZTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Compact Normal"))
				{
					compNormalTexture.ResourceName = "G_BUFFER_COMPACT_NORM_DEPTH";

					const char* items[] = { "Packed World Normal" };
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						compNormalTexture.ReservedIncludeMask = 0x00008880;

						compNormalTexture.ChannelMul[0] = 1.0f;
						compNormalTexture.ChannelMul[1] = 1.0f;
						compNormalTexture.ChannelMul[2] = 1.0f;
						compNormalTexture.ChannelMul[3] = 0.0f;

						compNormalTexture.ChannelAdd[0] = 0.0f;
						compNormalTexture.ChannelAdd[1] = 0.0f;
						compNormalTexture.ChannelAdd[2] = 0.0f;
						compNormalTexture.ChannelAdd[3] = 1.0f;

						compNormalTexture.PixelShaderGUID = m_ImGuiPixelShaderPackedLocalNormalGUID;
					}

					ImGui::Image(&compNormalTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Depth Stencil"))
				{
					depthStencilTexture.ResourceName = "G_BUFFER_DEPTH_STENCIL";

					depthStencilTexture.ReservedIncludeMask = 0x00008880;

					depthStencilTexture.ChannelMul[0] = 1.0f;
					depthStencilTexture.ChannelMul[1] = 1.0f;
					depthStencilTexture.ChannelMul[2] = 1.0f;
					depthStencilTexture.ChannelMul[3] = 0.0f;

					depthStencilTexture.ChannelAdd[0] = 0.0f;
					depthStencilTexture.ChannelAdd[1] = 0.0f;
					depthStencilTexture.ChannelAdd[2] = 0.0f;
					depthStencilTexture.ChannelAdd[3] = 1.0f;

					depthStencilTexture.PixelShaderGUID = m_ImGuiPixelShaderDepthGUID;

					ImGui::Image(&depthStencilTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Ray Tracing / SVGF"))
				{
					const char* items[] = { 
						"Direct Albedo",
						"Direct Radiance", 
						"Direct Radiance Reproj.", 
						"Direct Radiance Variance Est.",
						"Direct Radiance Feedback",
						"Indirect Albedo ",
						"Indirect Radiance ", 
						"Indirect Radiance Reproj.", 
						"Indirect Radiance Variance Est.",
						"Indirect Radiance Feedback",
						"Direct Variance", 
						"Indirect Variance", 
						"Accumulation" 
					};
					static int currentItem = 0;
					ImGui::ListBox("", &currentItem, items, IM_ARRAYSIZE(items), IM_ARRAYSIZE(items));

					if (currentItem == 0)
					{
						radianceTexture.ResourceName = "DIRECT_ALBEDO";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					if (currentItem == 1)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 2)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE_REPROJECTED";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 3)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE_VARIANCE_ESTIMATED";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 4)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE_FEEDBACK";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 5)
					{
						radianceTexture.ResourceName		= "INDIRECT_ALBEDO";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 6)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 7)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE_REPROJECTED";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 8)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE_VARIANCE_ESTIMATED";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 9)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE_FEEDBACK";
						radianceTexture.ReservedIncludeMask = 0x00008420;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 10)
					{
						radianceTexture.ResourceName		= "DIRECT_RADIANCE_REPROJECTED";
						radianceTexture.ReservedIncludeMask = 0x00001110;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 11)
					{
						radianceTexture.ResourceName		= "INDIRECT_RADIANCE_REPROJECTED";
						radianceTexture.ReservedIncludeMask = 0x00001110;

						radianceTexture.ChannelMul[0] = 1.0f;
						radianceTexture.ChannelMul[1] = 1.0f;
						radianceTexture.ChannelMul[2] = 1.0f;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}
					else if (currentItem == 12)
					{
						radianceTexture.ResourceName		= "SVGF_HISTORY";
						radianceTexture.ReservedIncludeMask = 0x00008880;

						const float32 maxHistoryLength = 32.0f;

						radianceTexture.ChannelMul[0] = 1.0f / maxHistoryLength;
						radianceTexture.ChannelMul[1] = 1.0f / maxHistoryLength;
						radianceTexture.ChannelMul[2] = 1.0f / maxHistoryLength;
						radianceTexture.ChannelMul[3] = 0.0f;

						radianceTexture.ChannelAdd[0] = 0.0f;
						radianceTexture.ChannelAdd[1] = 0.0f;
						radianceTexture.ChannelAdd[2] = 0.0f;
						radianceTexture.ChannelAdd[3] = 1.0f;

						radianceTexture.PixelShaderGUID = GUID_NONE;
					}

					ImGui::Image(&radianceTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Moments"))
				{
					momentsTexture.ResourceName = "MOMENTS";

					momentsTexture.ReservedIncludeMask = 0x00008421;

					momentsTexture.ChannelMul[0] = 1.0f;
					momentsTexture.ChannelMul[1] = 1.0f;
					momentsTexture.ChannelMul[2] = 1.0f;
					momentsTexture.ChannelMul[3] = 0.0f;

					momentsTexture.ChannelAdd[0] = 0.0f;
					momentsTexture.ChannelAdd[1] = 0.0f;
					momentsTexture.ChannelAdd[2] = 0.0f;
					momentsTexture.ChannelAdd[3] = 1.0f;

					momentsTexture.PixelShaderGUID = GUID_NONE;

					ImGui::Image(&momentsTexture, ImVec2(windowWidth, windowWidth / renderAspectRatio));

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	m_pScene->PrepareRender(pGraphicsCopyCommandList, pComputeCopyCommandList, m_pRenderer->GetFrameIndex(), delta);
	m_pRenderer->PrepareRender(delta);

	m_pRenderer->Render();
}

namespace LambdaEngine
{
	Game* CreateGame()
	{
		Sandbox* pSandbox = DBG_NEW Sandbox();        
		return pSandbox;
	}
}

bool Sandbox::InitRendererForDeferred()
{
	using namespace LambdaEngine;

	//GUID_Lambda geometryVertexShaderGUID		= ResourceManager::LoadShaderFromFile("GeometryDefVertex.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda geometryPixelShaderGUID			= ResourceManager::LoadShaderFromFile("GeometryDefPixel.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	//GUID_Lambda fullscreenQuadShaderGUID		= ResourceManager::LoadShaderFromFile("FullscreenQuad.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda shadingPixelShaderGUID			= ResourceManager::LoadShaderFromFile("ShadingDefPixel.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,			EShaderLang::GLSL);

	//GUID_Lambda raygenShaderGUID				= ResourceManager::LoadShaderFromFile("Raygen.glsl",				FShaderStageFlags::SHADER_STAGE_FLAG_RAYGEN_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda closestHitRadianceShaderGUID	= ResourceManager::LoadShaderFromFile("ClosestHitRadiance.glsl",	FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::GLSL);
	//GUID_Lambda missRadianceShaderGUID			= ResourceManager::LoadShaderFromFile("MissRadiance.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::GLSL);
	//GUID_Lambda closestHitShadowShaderGUID		= ResourceManager::LoadShaderFromFile("ClosestHitShadow.glsl",		FShaderStageFlags::SHADER_STAGE_FLAG_CLOSEST_HIT_SHADER,	EShaderLang::GLSL);
	//GUID_Lambda missShadowShaderGUID			= ResourceManager::LoadShaderFromFile("MissShadow.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_MISS_SHADER,			EShaderLang::GLSL);

	//GUID_Lambda postProcessShaderGUID			= ResourceManager::LoadShaderFromFile("PostProcess.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_COMPUTE_SHADER,		EShaderLang::GLSL);

	m_ImGuiPixelShaderNormalGUID			= ResourceManager::LoadShaderFromFile("ImGuiPixelNormal.frag",				FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelShaderDepthGUID				= ResourceManager::LoadShaderFromFile("ImGuiPixelDepth.frag",				FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelShaderMetallicGUID			= ResourceManager::LoadShaderFromFile("ImGuiPixelMetallic.frag",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelShaderRoughnessGUID			= ResourceManager::LoadShaderFromFile("ImGuiPixelRoughness.frag",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelShaderEmissiveGUID			= ResourceManager::LoadShaderFromFile("ImGuiPixelEmissive.frag",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelShaderPackedLocalNormalGUID	= ResourceManager::LoadShaderFromFile("ImGuiPixelPackedLocalNormal.frag",	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelLinearZGUID					= ResourceManager::LoadShaderFromFile("ImGuiPixelLinearZ.frag",				FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelCompactNormalFloatGUID		= ResourceManager::LoadShaderFromFile("ImGuiPixelCompactNormalFloat.frag",	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelShaderEmissionGUID			= ResourceManager::LoadShaderFromFile("ImGuiPixelEmission.frag",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelPackedMetallicGUID			= ResourceManager::LoadShaderFromFile("ImGuiPixelPackedMetallic.frag",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
	m_ImGuiPixelPackedRoughnessGUID			= ResourceManager::LoadShaderFromFile("ImGuiPixelPackedRoughness.frag",		FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);

	//ResourceManager::LoadShaderFromFile("ForwardVertex.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::GLSL);
	//ResourceManager::LoadShaderFromFile("ForwardPixel.glsl",			FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	EShaderLang::GLSL);

	//ResourceManager::LoadShaderFromFile("ShadingSimpleDefPixel.glsl",	FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER,	EShaderLang::GLSL);

	String renderGraphFile = "";

	if (SHOW_DEMO)
	{
		renderGraphFile = "../Assets/RenderGraphs/DEMO.lrg";
	}
	else
	{
		if constexpr(!RAY_TRACING_ENABLED && !POST_PROCESSING_ENABLED)
		{
			renderGraphFile = "../Assets/RenderGraphs/DEFERRED.lrg";
		}
		else if constexpr (RAY_TRACING_ENABLED && !SVGF_ENABLED && !POST_PROCESSING_ENABLED)
		{
			renderGraphFile = "../Assets/RenderGraphs/TRT_DEFERRED_SIMPLE.lrg";
		}
		else if constexpr (RAY_TRACING_ENABLED && SVGF_ENABLED && !POST_PROCESSING_ENABLED)
		{
			renderGraphFile = "../Assets/RenderGraphs/TRT_DEFERRED_SVGF.lrg";
		}
		else if constexpr (RAY_TRACING_ENABLED && POST_PROCESSING_ENABLED)
		{
			renderGraphFile = "../Assets/RenderGraphs/TRT_PP_DEFERRED.lrg";
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

	Window* pWindow	= CommonApplication::Get()->GetMainWindow();
	uint32 renderWidth	= pWindow->GetWidth();
	uint32 renderHeight = pWindow->GetHeight();

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

	if (RAY_TRACING_ENABLED)
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
			blueNoiseLUTFileNames[i] = "LUTs/BlueNoise/256_256/HDR_RGBA_" + std::string(str) + ".png";
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
	rendererDesc.pWindow			= CommonApplication::Get()->GetMainWindow();
	rendererDesc.BackBufferCount	= BACK_BUFFER_COUNT;
	
	m_pRenderer->Init(&rendererDesc);

	if (RENDERING_DEBUG_ENABLED)
	{
		ImGui::SetCurrentContext(ImGuiRenderer::GetImguiContext());
	}

	return true;
}