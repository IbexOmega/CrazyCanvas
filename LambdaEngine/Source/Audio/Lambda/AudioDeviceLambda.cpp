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
	}

	bool AudioDeviceLambda::LoadMusic(const char* pFilepath)
	{
		UNREFERENCED_VARIABLE(pFilepath);
		return false;
	}

	void AudioDeviceLambda::PlayMusic()
	{
	}

	void AudioDeviceLambda::PauseMusic()
	{
	}

	void AudioDeviceLambda::ToggleMusic()
	{
	}

	IAudioListener* AudioDeviceLambda::CreateAudioListener()
	{
		return nullptr;
	}

	ISoundEffect3D* AudioDeviceLambda::CreateSoundEffect() const
	{
		return DBG_NEW SoundEffect3DLambda(this);
	}

	ISoundInstance3D* AudioDeviceLambda::CreateSoundInstance() const
	{
		return DBG_NEW SoundInstance3DLambda(this);
	}

	IAudioGeometry* AudioDeviceLambda::CreateAudioGeometry() const
	{
		return nullptr;
	}

	IReverbSphere* AudioDeviceLambda::CreateReverbSphere() const
	{
		return nullptr;
	}
}
