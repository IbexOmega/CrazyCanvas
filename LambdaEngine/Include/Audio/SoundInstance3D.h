#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "SoundHelper.h"

struct FMOD_CHANNEL;

namespace LambdaEngine
{
	class AudioDevice;
	class SoundEffect3D;

	typedef unsigned int FMOD_MODE;

	struct SoundInstance3DDesc
	{
		const char* pName					= "SoundInstance3D";
		SoundEffect3D* pSoundEffect			= nullptr;
		ESoundModeFlags Flags				= ESoundModeFlags::SOUND_MODE_NONE;
	};

	class LAMBDA_API SoundInstance3D
	{
	public:
		DECL_REMOVE_COPY(SoundInstance3D);
		DECL_REMOVE_MOVE(SoundInstance3D);

		SoundInstance3D(const AudioDevice* pAudioDevice);
		~SoundInstance3D();

		/*
		* Initialize this SoundInstance3D
		*	desc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		bool Init(const SoundInstance3DDesc& desc);

		/*
		* Play the sound instance
		*/
		void Play();

		/*
		* Pause the sound instance
		*/
		void Pause();

		/*
		* Stop the sound instance, releases some internal resources, a consecutive call to Play will restart the sound instance
		*/
		void Stop();

		/*
		* Toggle the played/paused state of the sound instance
		*/
		void Toggle();

		/*
		* Set the world position of the sound instance
		*	position - The world space position, should be given in meters
		*/
		void SetPosition(const glm::vec3& position);

		/*
		* Set the volume of the sound instance in the range [-Inf, Inf]
		*/
		void SetVolume(float volume);

		/*
		* Set the pitch of the sound instance in the range [-Inf, Inf]
		*/
		void SetPitch(float pitch);

		const glm::vec3& GetPosition();
		float GetVolume();
		float GetPitch();

	private:
		bool IsPlaying();
		void RecreateHandleIfNeeded();

	private:
		//Engine
		const AudioDevice*	m_pAudioDevice;
		SoundEffect3D*		m_pSoundEffect;

		//FMOD
		FMOD_CHANNEL*		m_pChannel;
		FMOD_MODE			m_Mode;

		//Local
		const char*			m_pName;
		glm::vec3			m_Position;
		float				m_Volume;
		float				m_Pitch;
	};
}