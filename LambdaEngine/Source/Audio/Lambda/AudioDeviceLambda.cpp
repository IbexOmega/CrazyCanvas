#include "Audio/Lambda/AudioDeviceLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/SoundInstance3DLambda.h"

#include "Log/Log.h"

#include "portaudio.h"

namespace LambdaEngine
{	
	AudioDeviceLambda::AudioDeviceLambda()
	{
	}

	AudioDeviceLambda::~AudioDeviceLambda()
	{
		PaError result;
		
		result = Pa_Terminate();
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not terminate PortAudio, error: \"%s\"", Pa_GetErrorText(result));
		}
	}

	bool AudioDeviceLambda::Init(const AudioDeviceDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;
		m_MaxNumAudioListeners = pDesc->MaxNumAudioListeners;

		if (pDesc->MaxNumAudioListeners > 1)
		{
			LOG_ERROR("[AudioDeviceLambda]: MaxNumAudioListeners can not be greater than 1 in the current version of the Audio Engine");
			return false;
		}

		PaError result;

		result = Pa_Initialize();
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not initialize PortAudio, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		return true;
	}

	void AudioDeviceLambda::Tick()
	{
		for (auto it = m_SoundEffectsToDelete.begin(); it != m_SoundEffectsToDelete.end(); it++)
		{
			m_SoundEffects.erase(*it);
		}
		m_SoundEffectsToDelete.clear();

		for (auto it = m_SoundInstancesToDelete.begin(); it != m_SoundInstancesToDelete.end(); it++)
		{
			m_SoundInstances.erase(*it);
		}
		m_SoundInstancesToDelete.clear();

		for (auto it = m_SoundInstances.begin(); it != m_SoundInstances.end(); it++)
		{
			SoundInstance3DLambda* pSoundInstance = *it;

			pSoundInstance->UpdateVolume(m_MasterVolume, m_AudioListeners.GetData(), m_AudioListeners.GetSize());
		}
	}

	void AudioDeviceLambda::UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc)
	{
		auto it = m_AudioListenerMap.find(index);

		if (it == m_AudioListenerMap.end())
		{
			LOG_WARNING("[AudioDeviceLambda]: Audio Listener with index %u could not be found in device %s!", index, m_pName);
			return;
		}

		uint32 arrayIndex = it->second;
		m_AudioListeners[arrayIndex] = *pDesc;
	}

	uint32 AudioDeviceLambda::CreateAudioListener()
	{
		if (m_NumAudioListeners >= m_MaxNumAudioListeners)
		{
			LOG_WARNING("[AudioDeviceLambda]: Audio Listener could not be created, max amount reached for %s!", m_pName);
			return UINT32_MAX;
		}

		uint32 index = m_NumAudioListeners++;

		AudioListenerDesc audioListener = {};
		m_AudioListeners.PushBack(audioListener);

		m_AudioListenerMap[index] = m_AudioListeners.GetSize() - 1;

		return index;
	}

	IMusic* AudioDeviceLambda::CreateMusic(const MusicDesc* pDesc)
	{
		return nullptr;
	}

	ISoundEffect3D* AudioDeviceLambda::CreateSoundEffect(const SoundEffect3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundEffect3DLambda* pSoundEffect = DBG_NEW SoundEffect3DLambda(this);
		m_SoundEffects.insert(pSoundEffect);

		if (!pSoundEffect->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pSoundEffect;
		}
	}

	ISoundInstance3D* AudioDeviceLambda::CreateSoundInstance(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		SoundInstance3DLambda* pSoundInstance = DBG_NEW SoundInstance3DLambda(this);
		m_SoundInstances.insert(pSoundInstance);

		if (!pSoundInstance->Init(pDesc))
		{
			return nullptr;
		}
		else
		{
			return pSoundInstance;
		}
	}

	IAudioGeometry* AudioDeviceLambda::CreateAudioGeometry(const AudioGeometryDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		return nullptr;
	}

	IReverbSphere* AudioDeviceLambda::CreateReverbSphere(const ReverbSphereDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		return nullptr;
	}

	void AudioDeviceLambda::DeleteSoundEffect(SoundEffect3DLambda* pSoundEffect) const
	{
		m_SoundEffectsToDelete.insert(pSoundEffect);
	}

	void AudioDeviceLambda::DeleteSoundInstance(SoundInstance3DLambda* pSoundInstance) const
	{
		m_SoundInstancesToDelete.insert(pSoundInstance);
	}

	void AudioDeviceLambda::SetMasterVolume(float volume)
	{
		m_MasterVolume = volume;
	}
}
