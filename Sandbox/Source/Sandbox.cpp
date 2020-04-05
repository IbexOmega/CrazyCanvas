#include "Sandbox.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Network/API/PlatformSocketFactory.h"

#include "Resources/ResourceManager.h"

#include "Rendering/RenderSystem.h"

#include "Audio/AudioSystem.h"
#include "Audio/AudioListener.h"
#include "Audio/SoundEffect3D.h"
#include "Audio/SoundInstance3D.h"
#include "Audio/AudioGeometry.h"
#include "Audio/ReverbSphere.h"

Sandbox::Sandbox() : 
	m_pResourceManager(nullptr),
	m_pReverbSphere(nullptr),
	m_pAudioGeometry(nullptr)
{
	using namespace LambdaEngine;

	m_pResourceManager = new LambdaEngine::ResourceManager(LambdaEngine::RenderSystem::GetDevice(), LambdaEngine::AudioSystem::GetDevice());

	m_ToneSoundEffectGUID = m_pResourceManager->LoadSoundFromFile("../Assets/Sounds/noise.wav");
	m_GunSoundEffectGUID = m_pResourceManager->LoadSoundFromFile("../Assets/Sounds/GUN_FIRE-GoodSoundForYou.wav");

	m_pToneSoundEffect = m_pResourceManager->GetSound(m_ToneSoundEffectGUID);
	m_pGunSoundEffect = m_pResourceManager->GetSound(m_GunSoundEffectGUID);

	SoundInstance3DDesc soundInstanceDesc = {};
	soundInstanceDesc.pSoundEffect		= m_pToneSoundEffect;
	soundInstanceDesc.Flags				= ESoundModeFlags::SOUND_MODE_LOOPING;

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

Sandbox::~Sandbox()
{
	SAFEDELETE(m_pResourceManager);
	SAFEDELETE(m_pAudioListener);
	SAFEDELETE(m_pAudioGeometry);
}

void Sandbox::TestResourceManager()
{
	LOG_MESSAGE("\n-------Resource Handler Testing Start-------");

	

	////Audio Test
	//{
	//	GUID_Lambda failedSoundGUID = pResourceHandler->LoadSoundFromFile("THIS/SHOULD/FAIL.obj");
	//	GUID_Lambda successSoundGUID = pResourceHandler->LoadSoundFromFile("../Assets/Sounds/smb_gameover.wav");

	//	Sound* pSound = pResourceHandler->GetSound(successSoundGUID);
	//	pSound->Play();
	//}

	////Scene Test
	//{
	//	std::vector<GraphicsObject> sponzaGraphicsObjects;
	//	pResourceHandler->LoadSceneFromFile("../Assets/Scenes/sponza/", "sponza.obj", sponzaGraphicsObjects);
	//}

	////Mesh Test
	//{
	//	GUID_Lambda failedMeshGUID = pResourceHandler->LoadMeshFromFile("THIS/SHOULD/FAIL.obj");
	//	GUID_Lambda sucessMeshGUID = pResourceHandler->LoadMeshFromFile("../Assets/Meshes/bunny.obj");

	//	Mesh* pBunnyMesh = ResourceLoader::LoadMeshFromFile(pGraphicsDevice, "../Assets/Meshes/bunny.obj");

	//	SAFEDELETE(pBunnyMesh);
	//	SAFEDELETE(pResourceHandler);
	//}

	LOG_MESSAGE("-------Resource Handler Testing End-------\n");
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

	glm::vec3 tonePosition(glm::cos(m_Timer), 0.0f, glm::sin(m_Timer));
	m_pToneSoundInstance->SetPosition(tonePosition);
	//LOG_MESSAGE("Mario Position: %s", glm::to_string(marioPosition).c_str());
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
        Sandbox* pSandbox = new Sandbox();
        Input::AddKeyboardHandler(pSandbox);
        Input::AddMouseHandler(pSandbox);
        
        return pSandbox;
    }
}
