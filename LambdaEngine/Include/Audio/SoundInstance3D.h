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

		bool Init(const SoundInstance3DDesc& desc);

		void Play();
		void Pause();
		void Stop();
		void Toggle();

		void SetPosition(const glm::vec3& position);
		void SetVolume(float volume);
		void SetPitch(float pitch);

		const glm::vec3& GetPosition()	{ return m_Position; }
		float GetVolume()				{ return m_Volume; }
		float GetPitch()				{ return m_Pitch; }

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
		glm::vec3			m_Position;
		float				m_Volume;
		float				m_Pitch;
	};
}