#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundEffect3DLambda::SoundEffect3DLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
		UNREFERENCED_VARIABLE(pAudioDevice);
	}

	SoundEffect3DLambda::~SoundEffect3DLambda()
	{
		m_pAudioDevice->DeleteSoundEffect(this);

		SAFEDELETE_ARRAY(m_pWaveForm);
	}

	bool SoundEffect3DLambda::Init(const SoundEffect3DDesc* pDesc)
	{
        VALIDATE(pDesc);

		int32 result = WavLibLoadFileFloat32(pDesc->pFilepath, &m_pWaveForm, &m_Header, WAV_LIB_FLAG_MONO);
		if (result != WAVE_SUCCESS)
		{
			const char* pError = WavLibGetError(result);

			LOG_ERROR("[SoundEffect3DLambda]: Failed to load file '%s'. Error: %s", pDesc->pFilepath, pError);
			return false;
		}
		else
		{
			D_LOG_MESSAGE("[SoundEffect3DLambda]: Loaded file '%s'", pDesc->pFilepath);
			return true;
		}
	}

	void SoundEffect3DLambda::PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float volume, float pitch)
	{
		UNREFERENCED_VARIABLE(position);
		UNREFERENCED_VARIABLE(velocity);
		UNREFERENCED_VARIABLE(volume);
		UNREFERENCED_VARIABLE(pitch);
	}
}
