#pragma once

#include "LambdaEngine.h"

class FMOD_SYSTEM;
class FMOD_SOUND;
class FMOD_CHANNEL;

namespace LambdaEngine
{
	class AudioListener;
	class SoundEffect3D;
	class SoundInstance3D;
	class AudioGeometry;
	class ReverbSphere;

	struct AudioDeviceDesc
	{
		const char* pName			= "AudioDevice";
		bool Debug					= true;
		uint32 MaxNumAudioListeners	= 1;
		float MaxWorldSize			= 100.0f;
	};

	class LAMBDA_API AudioDevice
	{
	public:
		DECL_REMOVE_COPY(AudioDevice);
		DECL_REMOVE_MOVE(AudioDevice);

		AudioDevice();
		~AudioDevice();

		/*
		* Initialize this AudioDevice
		*	desc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		bool Init(const AudioDeviceDesc& desc);

		/*
		* Tick the audio device
		*/
		void Tick();

		/*
		* Load Music for streaming, only one music file can be loaded at any given time per AudioDevice
		*	pFilepath - A filepath to the audiofile
		* return - true if the initialization was successfull, otherwise returns false
		*/
		bool LoadMusic(const char* pFilepath);

		/*
		* Play the currently loaded music
		*/
		void PlayMusic();

		/*
		* Pause the currently loaded music
		*/
		void PauseMusic();

		/*
		* Toggle the played/paused state of the currently loaded music
		*/
		void ToggleMusic();
		
		AudioListener*		CreateAudioListener();
		SoundEffect3D*		CreateSound();
		SoundInstance3D*	CreateSoundInstance();
		AudioGeometry*		CreateAudioGeometry();
		ReverbSphere*		CreateReverbSphere();

	public:
		FMOD_SYSTEM* pSystem;

	private:
		const char*		m_pName;

		uint32			m_MaxNumAudioListeners;
		uint32			m_NumAudioListeners;

		FMOD_SOUND*		m_pMusicHandle;
		FMOD_CHANNEL*	m_pMusicChannel;
	};
}