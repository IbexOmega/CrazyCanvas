#include "Audio/FMOD/AudioDeviceFMOD.h"
#include "Audio/FMOD/MusicFMOD.h"
#include "Audio/FMOD/SoundEffect3DFMOD.h"
#include "Audio/FMOD/SoundEffect2DFMOD.h"
#include "Audio/FMOD/SoundInstance3DFMOD.h"
#include "Audio/FMOD/SoundInstance2DFMOD.h"
#include "Audio/FMOD/AudioGeometryFMOD.h"
#include "Audio/FMOD/ReverbSphereFMOD.h"

#include "Log/Log.h"

#include "Threading/API/Thread.h"

//#define FMOD_LOGS_ENABLED

namespace LambdaEngine
{
	FMOD_RESULT DebugCallback(FMOD_DEBUG_FLAGS severity, const char* pFile, int line, const char* pFunc, const char* pMessage)
	{
		if (severity & FMOD_DEBUG_LEVEL_ERROR)
		{
			LOG_ERROR("[%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
			return FMOD_OK;
		}

#ifdef FMOD_LOGS_ENABLED
		if (severity & FMOD_DEBUG_LEVEL_WARNING)
		{
			LOG_WARNING("[%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
			return FMOD_OK;
		}

		if (!(severity & FMOD_DEBUG_LEVEL_LOG))
		{
			LOG_DEBUG("[%s : %u : %s] - \"%s\"", pFile, line, pFunc, pMessage);
			return FMOD_OK;
		}
#endif

		return FMOD_OK;
	}

	AudioDeviceFMOD::AudioDeviceFMOD()
	{

	}

	AudioDeviceFMOD::~AudioDeviceFMOD()
	{
		if (pSystem != nullptr)
		{
			if (FMOD_System_Release(pSystem) != FMOD_OK)
			{
				LOG_ERROR("FMOD System could not be released for %s", m_pName);
			}
			else
			{
				LOG_DEBUG("FMOD System released successfully for %s", m_pName);
			}

			pSystem = nullptr;
		}
	}

	bool AudioDeviceFMOD::Init(const AudioDeviceDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;
		m_MaxNumAudioListeners = pDesc->MaxNumAudioListeners;

		if (pDesc->Debug)
		{
			FMOD_DEBUG_FLAGS debugLevel		= FMOD_DEBUG_LEVEL_LOG;
			FMOD_DEBUG_MODE debugMode		= FMOD_DEBUG_MODE_CALLBACK;

			if (FMOD_Debug_Initialize(debugLevel, debugMode, DebugCallback, nullptr) != FMOD_OK)
			{
				LOG_WARNING("FMOD Debug Mode could not be initialized");
				return false;
			}
		}

		if (FMOD_System_Create(&pSystem) != FMOD_OK)
		{
			LOG_ERROR("FMOD System could not be created for %s", m_pName);
			return false;
		}

		int numChannels				= 512;
		FMOD_INITFLAGS initFlags	= FMOD_INIT_NORMAL;

		if (FMOD_System_Init(pSystem, numChannels, initFlags, nullptr) != FMOD_OK)
		{
			LOG_ERROR("FMOD System could not be initialized for %s", m_pName);
			return false;
		}

		if (FMOD_System_SetGeometrySettings(pSystem, pDesc->MaxWorldSize) != FMOD_OK)
		{
			LOG_ERROR("FMOD Geometry Settings could not be set for %s", m_pName);
			return false;
		}

		// Set 3D sound settings
		float dopparScale = 1.0f, distanceFactor = 0.5f, rolloffScale = 1.0f;
		if (FMOD_System_Set3DSettings(pSystem, dopparScale, distanceFactor, rolloffScale) != FMOD_OK)
		{
			LOG_ERROR("FMOD 3D Settings could not be set for %s", m_pName);
			return false;
		}

		LOG_DEBUG("Successfully initialized %s!", m_pName);

		return true;
	}

	void AudioDeviceFMOD::Tick()
	{
		FMOD_System_Update(pSystem);
	}

	void AudioDeviceFMOD::UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc)
	{
		FMOD_VECTOR fmodPosition	= { pDesc->Position.x,		pDesc->Position.y,		pDesc->Position.z };
		FMOD_VECTOR fmodVelocity	= { 0.0f,					0.0f,					0.0f };
		FMOD_VECTOR fmodForward		= { pDesc->Forward.x,		pDesc->Forward.y,		pDesc->Forward.z };
		FMOD_VECTOR fmodUp			= { pDesc->Up.x,			pDesc->Up.y,			pDesc->Up.z };

		FMOD_System_Set3DListenerAttributes(pSystem, index, &fmodPosition, &fmodVelocity, &fmodForward, &fmodUp);
	}

	uint32 AudioDeviceFMOD::GetAudioListener(bool requestNew)
	{
		if (requestNew)
		{
			if (m_NumAudioListeners < m_MaxNumAudioListeners)
			{
				return m_NumAudioListeners++;
			}
			else
			{
				LOG_WARNING("Audio Listener could not be created, max amount reached for %s!", m_pName);
				return UINT32_MAX;
			}
		}

		return 0;
	}

	IMusic* AudioDeviceFMOD::CreateMusic(const MusicDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		MusicFMOD* pMusic = DBG_NEW MusicFMOD(this);

		if (!pMusic->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pMusic;
		}
	}

	ISoundEffect3D* AudioDeviceFMOD::Create3DSoundEffect(const SoundEffect3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundEffect3DFMOD* pSoundEffect = DBG_NEW SoundEffect3DFMOD(this);

		if (!pSoundEffect->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pSoundEffect;
		}
	}

	ISoundEffect2D* AudioDeviceFMOD::Create2DSoundEffect(const SoundEffect2DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundEffect2DFMOD* pSoundEffect = DBG_NEW SoundEffect2DFMOD(this);

		if (!pSoundEffect->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pSoundEffect;
		}
	}

	ISoundInstance3D* AudioDeviceFMOD::Create3DSoundInstance(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundInstance3DFMOD* pSoundInstance = DBG_NEW SoundInstance3DFMOD(this);

		if (!pSoundInstance->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pSoundInstance;
		}
	}

	ISoundInstance2D* AudioDeviceFMOD::Create2DSoundInstance(const SoundInstance2DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundInstance2DFMOD* pSoundInstance = DBG_NEW SoundInstance2DFMOD(this);

		if (!pSoundInstance->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pSoundInstance;
		}
	}


	IAudioGeometry* AudioDeviceFMOD::CreateAudioGeometry(const AudioGeometryDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		AudioGeometryFMOD* pAudioGeometry = DBG_NEW AudioGeometryFMOD(this);

		if (!pAudioGeometry->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pAudioGeometry;
		}
	}

	IReverbSphere* AudioDeviceFMOD::CreateReverbSphere(const ReverbSphereDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		ReverbSphereFMOD* pReverbSphere = DBG_NEW ReverbSphereFMOD(this);

		if (!pReverbSphere->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pReverbSphere;
		}
	}

	void AudioDeviceFMOD::SetMasterVolume(float volume)
	{
		FMOD_CHANNELGROUP* pChannelGroup;
		FMOD_System_GetMasterChannelGroup(pSystem, &pChannelGroup);
		FMOD_ChannelGroup_SetVolume(pChannelGroup, volume);
	}

	float AudioDeviceFMOD::GetMasterVolume() const
	{
		FMOD_CHANNELGROUP* pChannelGroup;
		float volume = -1.0f;
		FMOD_System_GetMasterChannelGroup(pSystem, &pChannelGroup);
		FMOD_ChannelGroup_GetVolume(pChannelGroup, &volume);

		return volume;
	}
};
