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

Sandbox::Sandbox() : 
	m_pResourceManager(nullptr)
{
	using namespace LambdaEngine;

	m_pResourceManager = new LambdaEngine::ResourceManager(LambdaEngine::RenderSystem::GetDevice(), LambdaEngine::AudioSystem::GetDevice());

	m_ToneSoundEffectGUID = m_pResourceManager->LoadSoundFromFile("../Assets/Sounds/noise.wav");
	m_GunSoundEffectGUID = m_pResourceManager->LoadSoundFromFile("../Assets/Sounds/9_mm_gunshot-mike-koenig-123.wav");

	m_pToneSoundEffect = m_pResourceManager->GetSound(m_ToneSoundEffectGUID);
	m_pGunSoundEffect = m_pResourceManager->GetSound(m_GunSoundEffectGUID);

	SoundInstance3DDesc soundInstanceDesc = {};
	soundInstanceDesc.pSoundEffect		= m_pToneSoundEffect;
	soundInstanceDesc.Flags				= ESoundFlags::LOOPING;

	m_pToneSoundInstance = AudioSystem::GetDevice()->CreateSoundInstance();
	m_pToneSoundInstance->Init(soundInstanceDesc);
	m_pToneSoundInstance->SetVolume(0.5f);

	m_SpawnPlayAts = false;
	m_GunshotTimer = 0.0f;
	m_GunshotDelay = 1.0f;
	m_Timer = 0.0f;

	m_pAudioListener = AudioSystem::GetDevice()->CreateAudioListener();
}

Sandbox::~Sandbox()
{
	SAFEDELETE(m_pResourceManager);
	SAFEDELETE(m_pAudioListener);
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
	LOG_MESSAGE("Key Pressed: %d", key);

	using namespace LambdaEngine;

	static glm::vec3 pTestAudioPosition = glm::vec3(0.0f);

	if (key == EKey::KEY_KP_1)
	{
		m_pToneSoundInstance->Toggle();
	}
	else if (key == EKey::KEY_KP_2)
	{
		m_SpawnPlayAts = !m_SpawnPlayAts;
	}
	else if (key == EKey::KEY_KP_ADD)
	{
		m_GunshotDelay += 0.05f;
	}
	else if (key == EKey::KEY_KP_SUBTRACT)
	{
		m_GunshotDelay = glm::max(m_GunshotDelay - 0.05f, 0.125f);
	}
}

void Sandbox::OnKeyHeldDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Held Down: %d", key);
}

void Sandbox::OnKeyUp(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Released: %d", key);
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
