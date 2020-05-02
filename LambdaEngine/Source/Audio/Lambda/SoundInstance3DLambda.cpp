#include "Audio/Lambda/SoundInstance3DLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"
#include "Audio/Lambda/AudioDeviceLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundInstance3DLambda::SoundInstance3DLambda(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceLambda*>(pAudioDevice))
	{
	}

	SoundInstance3DLambda::~SoundInstance3DLambda()
	{
		m_pAudioDevice->DeleteSoundInstance(this);
		
		SAFEDELETE_ARRAY(m_pWaveForm);

		PaError result;

		result = Pa_CloseStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not close PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
		}
	}

	bool SoundInstance3DLambda::Init(const SoundInstance3DDesc* pDesc)
	{
		VALIDATE(pDesc);

		const SoundEffect3DLambda* pSoundEffect = reinterpret_cast<const SoundEffect3DLambda*>(pDesc->pSoundEffect);

		m_CurrentBufferIndex		= 0;
		m_SampleCount		= pSoundEffect->GetSampleCount();
		m_ChannelCount		= pSoundEffect->GetChannelCount();
		m_TotalSampleCount	= m_SampleCount * m_ChannelCount;
		m_pWaveForm			= new float32[m_TotalSampleCount];
		memcpy(m_pWaveForm, pSoundEffect->GetWaveform(), sizeof(float32) * m_TotalSampleCount);

		PaError result;

		/* Open an audio I/O stream. */
		result = Pa_OpenDefaultStream(
			&m_pStream,
			0,          /* no input channels */
			m_ChannelCount,          /* stereo output */
			paFloat32,  /* 32 bit floating point output */
			pSoundEffect->GetSampleRate(),
			128,	/* frames per buffer, i.e. the number
							   of sample frames that PortAudio will
							   request from the callback. Many apps
							   may want to use
							   paFramesPerBufferUnspecified, which
							   tells PortAudio to pick the best,
							   possibly changing, buffer size.*/
			PortAudioCallback, /* this is your callback function */
			this); /*This is a pointer that will be passed to your callback*/

		if (result != paNoError)
		{
			LOG_ERROR("[SoundInstance3DLambda]: Could not open PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		result = Pa_StartStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[SoundInstance3DLambda]: Could not start PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
			return false;
		}

		return true;
	}

	void SoundInstance3DLambda::Play()
	{
	}

	void SoundInstance3DLambda::Pause()
	{
	}

	void SoundInstance3DLambda::Stop()
	{
	}

	void SoundInstance3DLambda::Toggle()
	{
	}

	void SoundInstance3DLambda::SetPosition(const glm::vec3& position)
	{
		m_Position = position;
	}

	void SoundInstance3DLambda::SetVolume(float volume)
	{
		m_Volume = volume;
	}

	void SoundInstance3DLambda::SetPitch(float pitch)
	{
		UNREFERENCED_VARIABLE(pitch);
	}

	const glm::vec3& SoundInstance3DLambda::GetPosition() const
	{
		return m_Position;
	}

	float SoundInstance3DLambda::GetVolume() const
	{
		return m_Volume;
	}

	float SoundInstance3DLambda::GetPitch() const
	{
		return 1.0f;
	}

	void SoundInstance3DLambda::UpdateVolume(float masterVolume, const AudioListenerDesc* pAudioListeners, uint32 count)
	{
	}

	int32 SoundInstance3DLambda::LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer)
	{
		for (uint32 f = 0; f < framesPerBuffer; f++)
		{
			for (uint32 c = 0; c < m_ChannelCount; c++)
			{
				float sample = m_pWaveForm[m_CurrentBufferIndex++];
				(*(pOutputBuffer++)) = m_OutputVolume * sample;
			}

			if (m_CurrentBufferIndex == m_TotalSampleCount)
				m_CurrentBufferIndex = 0;
		}

		return paNoError;
	}

	int32 SoundInstance3DLambda::PortAudioCallback(
		const void* pInputBuffer, 
		void* pOutputBuffer,
		unsigned long framesPerBuffer, 
		const PaStreamCallbackTimeInfo* pTimeInfo, 
		PaStreamCallbackFlags statusFlags,
		void* pUserData)
	{
		UNREFERENCED_VARIABLE(pInputBuffer);
		UNREFERENCED_VARIABLE(pTimeInfo);
		UNREFERENCED_VARIABLE(statusFlags);

		SoundInstance3DLambda* pInstance = reinterpret_cast<SoundInstance3DLambda*>(pUserData);
		VALIDATE(pInstance != nullptr);

		return pInstance->LocalAudioCallback(reinterpret_cast<float32*>(pOutputBuffer), framesPerBuffer);
	}
}