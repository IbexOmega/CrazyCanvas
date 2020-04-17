#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/FMOD/AudioListenerFMOD.h"
#include "Audio/FMOD/SoundEffect3DFMOD.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"
#include "Audio/FMOD/AudioGeometryFMOD.h"
#include "Audio/FMOD/ReverbSphereFMOD.h"

#include "Log/Log.h"

#include "Threading/API/Thread.h"

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

	AudioDeviceFMOD::AudioDeviceFMOD() :
		pSystem(nullptr),
		m_MaxNumAudioListeners(0),
		m_NumAudioListeners(0),
		m_pMusicHandle(nullptr),
		m_pMusicChannel(nullptr)
	{
		
	}

	AudioDeviceFMOD::~AudioDeviceFMOD()
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
				LOG_ERROR("[AudioDeviceFMOD]: FMOD System could not be released for %s", m_pName);
			}
			else
			{
				D_LOG_MESSAGE("[AudioDeviceFMOD]: FMOD System released successfully for %s", m_pName);
			}

			pSystem = nullptr;
		}
	}

	bool AudioDeviceFMOD::Init(const AudioDeviceDesc& desc)
	{
		m_pName = desc.pName;
		m_MaxNumAudioListeners = desc.MaxNumAudioListeners;

		if (desc.Debug)
		{
			FMOD_DEBUG_FLAGS debugLevel		= FMOD_DEBUG_LEVEL_LOG;
			FMOD_DEBUG_MODE debugMode		= FMOD_DEBUG_MODE_CALLBACK;

			if (FMOD_Debug_Initialize(debugLevel, debugMode, DebugCallback, nullptr) != FMOD_OK)
			{
				LOG_WARNING("[AudioDeviceFMOD]: FMOD Debug Mode could not be initialized");
				return false;
			}
		}

		if (FMOD_System_Create(&pSystem) != FMOD_OK)
		{
			LOG_ERROR("[AudioDeviceFMOD]: FMOD System could not be created for %s", m_pName);
			return false;
		}

		int numChannels				= 512;
		FMOD_INITFLAGS initFlags	= FMOD_INIT_NORMAL;

		if (FMOD_System_Init(pSystem, numChannels, initFlags, nullptr) != FMOD_OK)
		{
			LOG_ERROR("[AudioDeviceFMOD]: FMOD System could not be initialized for %s", m_pName);
			return false;
		}

		if (FMOD_System_SetGeometrySettings(pSystem, desc.MaxWorldSize) != FMOD_OK)
		{
			LOG_ERROR("[AudioDeviceFMOD]: FMOD Geometry Settings could not be set for %s", m_pName);
			return false;
		}

		D_LOG_MESSAGE("[AudioDeviceFMOD]: Successfully initialized %s!", m_pName);

		return true;
	}

	void AudioDeviceFMOD::Tick()
	{
		FMOD_System_Update(pSystem);
	}

	bool AudioDeviceFMOD::LoadMusic(const char* pFilepath)
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
			LOG_WARNING("[AudioDeviceFMOD]: Music \"%s\" could not be initialized in %s", pFilepath, m_pName);
			return false;
		}

		if (FMOD_System_PlaySound(pSystem, m_pMusicHandle, nullptr, true, &m_pMusicChannel) != FMOD_OK)
		{
			LOG_WARNING("[AudioDeviceFMOD]: Music \"%s\" could not be played in %s", pFilepath, m_pName);
			return false;
		}

		D_LOG_MESSAGE("[AudioDeviceFMOD]: Loaded Music \"%s\" in %s", pFilepath, m_pName);
		
		return true;
	}

	void AudioDeviceFMOD::PlayMusic()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pMusicChannel, &isPlaying);

		if (isPlaying)
		{
			FMOD_Channel_SetPaused(m_pMusicChannel, 0);
		}
	}

	void AudioDeviceFMOD::PauseMusic()
	{
		FMOD_BOOL isPlaying = 0;
		FMOD_Channel_IsPlaying(m_pMusicChannel, &isPlaying);

		if (isPlaying)
		{
			FMOD_Channel_SetPaused(m_pMusicChannel, 1);
		}
	}

	void AudioDeviceFMOD::ToggleMusic()
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

	IAudioListener* AudioDeviceFMOD::CreateAudioListener()
	{
		if (m_NumAudioListeners >= m_MaxNumAudioListeners)
		{
			LOG_WARNING("[AudioDeviceFMOD]: AudioListenerFMOD could not be created, max amount reached for %s!", m_pName);
			return nullptr;
		}

		AudioListenerFMOD* pAudioListener = DBG_NEW AudioListenerFMOD(this);

		AudioListenerDesc audioListenerDesc = {};
		audioListenerDesc.ListenerIndex = m_NumAudioListeners++;
		pAudioListener->Init(audioListenerDesc);

		return pAudioListener;
	}

	ISoundEffect3D* AudioDeviceFMOD::CreateSoundEffect() const
	{
		return DBG_NEW SoundEffect3DFMOD(this);
	}

	ISoundInstance3D* AudioDeviceFMOD::CreateSoundInstance() const
	{
		return DBG_NEW SoundInstance3DFMOD(this);
	}

	IAudioGeometry* AudioDeviceFMOD::CreateAudioGeometry() const
	{
		return DBG_NEW AudioGeometryFMOD(this);
	}

	IReverbSphere* AudioDeviceFMOD::CreateReverbSphere() const
	{
		return DBG_NEW ReverbSphereFMOD(this);
	}
};
