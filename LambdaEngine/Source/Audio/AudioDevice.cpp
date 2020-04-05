#include "Audio/AudioDevice.h"
#include "Audio/Audio.h"

#include "Audio/AudioListener.h"
#include "Audio/SoundEffect3D.h"
#include "Audio/SoundInstance3D.h"

#include "Log/Log.h"
#include "Threading/Thread.h"

namespace LambdaEngine
{
	FMOD_RESULT DebugCallback(FMOD_DEBUG_FLAGS severity, const char* pFile, int line, const char* pFunc, const char* pMessage)
	{
		if (severity & FMOD_DEBUG_LEVEL_WARNING)
		{
			LOG_WARNING("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}
		else if (severity & FMOD_DEBUG_LEVEL_ERROR)
		{
			LOG_ERROR("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}
		else if (!(severity & FMOD_DEBUG_LEVEL_LOG))
		{
			D_LOG_INFO("[FMOD_DEBUG]: [%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
		}

		return FMOD_OK;
	}

	AudioDevice::AudioDevice() :
		pSystem(nullptr),
		m_MaxNumAudioListeners(0),
		m_NumAudioListeners(0),
		m_pMusicHandle(nullptr),
		m_pMusicChannel(nullptr)
	{
		
	}

	AudioDevice::~AudioDevice()
	{
		if (m_pMusicHandle != nullptr)
		{
			FMOD_Channel_Stop(m_pMusicChannel);
			FMOD_Sound_Release(m_pMusicHandle);
			m_pMusicHandle = nullptr;
		}
		
		if (pSystem != nullptr)
		{
			if (FMOD_System_Release(pSystem) != FMOD_OK)
			{
				LOG_ERROR("[AudioDevice]: FMOD System could not be released");
			}
			else
			{
				D_LOG_MESSAGE("[AudioDevice]: FMOD System released successfully");
			}
		}
	}

	bool AudioDevice::Init(const AudioDeviceDesc& desc)
	{
		m_MaxNumAudioListeners = desc.MaxNumAudioListeners;

		if (desc.Debug)
		{
			FMOD_DEBUG_FLAGS debugLevel		= FMOD_DEBUG_LEVEL_LOG;
			FMOD_DEBUG_MODE debugMode		= FMOD_DEBUG_MODE_CALLBACK;

			if (FMOD_Debug_Initialize(debugLevel, debugMode, DebugCallback, nullptr) != FMOD_OK)
			{
				LOG_WARNING("[AudioDevice]: FMOD Debug Mode could not be initialized");
				return false;
			}
		}

		if (FMOD_System_Create(&pSystem) != FMOD_OK)
		{
			LOG_ERROR("[AudioDevice]: FMOD System could not be created");
			return false;
		}

		int numChannels				= 512;
		FMOD_INITFLAGS initFlags	= FMOD_INIT_NORMAL;

		if (FMOD_System_Init(pSystem, numChannels, initFlags, nullptr) != FMOD_OK)
		{
			LOG_ERROR("[AudioDevice]: FMOD System could not be created");
			return false;
		}

		D_LOG_MESSAGE("[AudioDevice]: Successfully initialized!");

		return true;
	}

	void AudioDevice::Tick()
	{
		FMOD_System_Update(pSystem);
	}

	bool AudioDevice::LoadMusic(const char* pFilepath)
	{
		if (m_pMusicHandle != nullptr)
		{
			FMOD_Sound_Release(m_pMusicHandle);
			m_pMusicHandle = nullptr;
		}
		
		FMOD_MODE mode = FMOD_2D | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM;

		FMOD_CREATESOUNDEXINFO soundCreateInfo = {};
		soundCreateInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		
		if (FMOD_System_CreateSound(pSystem, pFilepath, mode, &soundCreateInfo, &m_pMusicHandle) != FMOD_OK)
		{
			LOG_WARNING("[AudioDevice]: Music \"%s\" could not be initialized", pFilepath);
			return false;
		}

		if (FMOD_System_PlaySound(pSystem, m_pMusicHandle, nullptr, true, &m_pMusicChannel) != FMOD_OK)
		{
			LOG_WARNING("[AudioDevice]: Music \"%s\" could not be played", pFilepath);
			return false;
		}

		D_LOG_MESSAGE("[AudioDevice]: Loaded Music \"%s\"", pFilepath);
		
		return true;
	}

	void AudioDevice::PlayMusic()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pMusicChannel, &isPlaying);

		if (isPlaying)
		{
			FMOD_Channel_SetPaused(m_pMusicChannel, 0);
		}
	}

	void AudioDevice::PauseMusic()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pMusicChannel, &isPlaying);

		if (isPlaying)
		{
			FMOD_Channel_SetPaused(m_pMusicChannel, 1);
		}
	}

	void AudioDevice::ToggleMusic()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pMusicChannel, &isPlaying);

		if (isPlaying)
		{
			FMOD_BOOL isPaused = 0;
			FMOD_Channel_GetPaused(m_pMusicChannel, &isPaused);
			FMOD_Channel_SetPaused(m_pMusicChannel, (isPaused ^ 0x1));
		}
	}

	AudioListener* AudioDevice::CreateAudioListener()
	{
		if (m_NumAudioListeners >= m_MaxNumAudioListeners)
		{
			LOG_WARNING("[AudioDevice]: AudioListener could not be created, max amount reached!");
			return nullptr;
		}

		AudioListener* pAudioListener = DBG_NEW AudioListener(this);

		AudioListenerDesc audioListenerDesc = {};
		audioListenerDesc.ListenerIndex = m_NumAudioListeners++;
		pAudioListener->Init(audioListenerDesc);

		return pAudioListener;
	}

	SoundEffect3D* AudioDevice::CreateSound()
	{
		return DBG_NEW SoundEffect3D(this);
	}

	SoundInstance3D* AudioDevice::CreateSoundInstance()
	{
		return DBG_NEW SoundInstance3D(this);
	}

};