#include "Audio/Lambda/SoundInstance3DLambda.h"
#include "Audio/Lambda/SoundEffect3DLambda.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	SoundInstance3DLambda::SoundInstance3DLambda(const IAudioDevice* pAudioDevice)
	{
		UNREFERENCED_VARIABLE(pAudioDevice);
	}

	SoundInstance3DLambda::~SoundInstance3DLambda()
	{
		PaError result;

		result = Pa_CloseStream(m_pStream);
		if (result != paNoError)
		{
			LOG_ERROR("[AudioDeviceLambda]: Could not close PortAudio stream, error: \"%s\"", Pa_GetErrorText(result));
		}
	}

	bool SoundInstance3DLambda::Init(const SoundInstance3DDesc& desc)
	{
		SoundEffect3DLambda* pSoundEffect = reinterpret_cast<SoundEffect3DLambda*>(desc.pSoundEffect);

		m_CurrentSample = 0;
		m_SampleCount	= pSoundEffect->GetSampleCount();
		m_ChannelCount	= pSoundEffect->GetChannelCount();
		m_pWaveForm		= new float32[m_SampleCount * m_ChannelCount];
		memcpy(m_pWaveForm, pSoundEffect->GetWaveform(), sizeof(float32) * m_SampleCount * m_ChannelCount);

		PaError result;

		/* Open an audio I/O stream. */
		result = Pa_OpenDefaultStream(
			&m_pStream,
			0,          /* no input channels */
			m_ChannelCount,          /* stereo output */
			paFloat32,  /* 32 bit floating point output */
			pSoundEffect->GetSampleRate(),
			256,			/* frames per buffer, i.e. the number
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
		UNREFERENCED_VARIABLE(position);
	}

	void SoundInstance3DLambda::SetVolume(float volume)
	{
		UNREFERENCED_VARIABLE(volume);
	}

	void SoundInstance3DLambda::SetPitch(float pitch)
	{
		UNREFERENCED_VARIABLE(pitch);
	}

	const glm::vec3& SoundInstance3DLambda::GetPosition()
	{
		static glm::vec3 temp;
		return temp;
	}

	float SoundInstance3DLambda::GetVolume()
	{
		return 1.0f;
	}

	float SoundInstance3DLambda::GetPitch()
	{
		return 1.0f;
	}

	void SoundInstance3DLambda::LocalAudioCallback(float* pOutputBuffer, unsigned long framesPerBuffer)
	{
		for (uint32 f = 0; f < framesPerBuffer / m_ChannelCount; f++)
		{
			for (uint32 c = 0; c < m_ChannelCount; c++)
			{
				float sample = m_pWaveForm[m_CurrentSample * m_ChannelCount + c];
				(*(pOutputBuffer++)) = sample;	
			}

			m_CurrentSample++;
			m_CurrentSample = m_CurrentSample % (m_SampleCount * m_ChannelCount);
		}
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

		if (pUserData != nullptr)
		{
			reinterpret_cast<SoundInstance3DLambda*>(pUserData)->LocalAudioCallback(
				reinterpret_cast<float32*>(pOutputBuffer), framesPerBuffer);
		}

		return paNoError;
	}
}