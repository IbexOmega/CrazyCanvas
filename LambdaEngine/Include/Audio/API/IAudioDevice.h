#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IAudioListener;
	class ISoundEffect3D;
	class ISoundInstance3D;
	class IAudioGeometry;
	class IReverbSphere;
	
	enum class EAudioAPI
	{
		FMOD,
		LAMBDA
	};
	
	struct AudioDeviceDesc
	{
		const char* pName				= "Audio Device";
		bool Debug						= true;
		uint32 MaxNumAudioListeners		= 1;
		float MaxWorldSize				= 100.0f;
	};
	
	class IAudioDevice
	{
	public:
		DECL_INTERFACE(IAudioDevice);
		
		/*
		* Initialize this AudioDeviceFMOD
		*	desc - A description of initialization parameters
		* return - true if the initialization was successful, otherwise returns false
		*/
		virtual bool Init(const AudioDeviceDesc& desc) = 0;

		/*
		* Tick the audio device
		*/
		virtual void Tick() = 0;

		/*
		* Load Music for streaming, only one music file can be loaded at any given time per AudioDeviceFMOD
		*	pFilepath - A filepath to the audiofile
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool LoadMusic(const char* pFilepath) = 0;

		/*
		* Play the currently loaded music
		*/
		virtual void PlayMusic() = 0;

		/*
		* Pause the currently loaded music
		*/
		virtual void PauseMusic() = 0;

		/*
		* Toggle the played/paused state of the currently loaded music
		*/
		virtual void ToggleMusic() = 0;

		virtual IAudioListener*		CreateAudioListener()	= 0;
		virtual ISoundEffect3D*		CreateSoundEffect()		const = 0;
		virtual ISoundInstance3D*	CreateSoundInstance()	const = 0;
		virtual IAudioGeometry*		CreateAudioGeometry()	const = 0;
		virtual IReverbSphere*		CreateReverbSphere()	const = 0;
	};

	LAMBDA_API IAudioDevice* CreateAudioDevice(EAudioAPI api, const AudioDeviceDesc& desc);
}
