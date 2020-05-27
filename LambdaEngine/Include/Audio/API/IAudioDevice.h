#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct MusicDesc;
	struct AudioGeometryDesc;
	struct ReverbSphereDesc;
	struct SoundEffect3DDesc;
	struct SoundInstance3DDesc;

	class IMusic;
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
		const char*		pName					= "Audio Device";
		float			MasterVolume			= 1.0f;
		bool			Debug					= true;
		uint32			MaxNumAudioListeners	= 1;
		float			MaxWorldSize			= 100.0f;
	};
	
	struct AudioListenerDesc
	{
		glm::vec3	Position					= glm::vec3(0.0f);
		glm::vec3	Forward						= glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3	Up							= glm::vec3(0.0f, 1.0f, 0.0f);
		float		Volume						= 1.0f;
	};

	class IAudioDevice
	{
	public:
		DECL_INTERFACE(IAudioDevice);
		
		/*
		* Initialize this AudioDeviceFMOD
		*	pDesc - A description of initialization parameters
		* return - true if the initialization was successful, otherwise returns false
		*/
		virtual bool Init(const AudioDeviceDesc* pDesc) = 0;

		/*
		* Tick the audio device
		*/
		virtual void Tick() = 0;

		virtual void UpdateAudioListener(uint32 index, const AudioListenerDesc* pDesc) = 0;

		virtual uint32				CreateAudioListener()									= 0;
		virtual IMusic*				CreateMusic(const MusicDesc* pDesc)						= 0;
		virtual ISoundEffect3D*		CreateSoundEffect(const SoundEffect3DDesc* pDesc)		= 0;
		virtual ISoundInstance3D*	CreateSoundInstance(const SoundInstance3DDesc* pDesc)	= 0;
		virtual IAudioGeometry*		CreateAudioGeometry(const AudioGeometryDesc* pDesc)		= 0;
		virtual IReverbSphere*		CreateReverbSphere(const ReverbSphereDesc* pDesc)		= 0;

		virtual void SetMasterVolume(float volume) = 0;

		virtual float GetMasterVolume() const = 0;
	};

	LAMBDA_API IAudioDevice* CreateAudioDevice(EAudioAPI api, const AudioDeviceDesc& desc);
}
