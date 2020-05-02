#pragma once

#include "FMOD.h"
#include "Audio/API/ISoundEffect3D.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class SoundEffect3DFMOD : public ISoundEffect3D
	{
	public:
		SoundEffect3DFMOD(const IAudioDevice* pAudioDevice);
		~SoundEffect3DFMOD();

		virtual bool Init(const SoundEffect3DDesc* pDesc) override final;
		virtual void PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float volume, float pitch) override final;
		
		FMOD_SOUND* GetHandle();
		uint32 GetLengthMS();

	private:
		//Engine
		const AudioDeviceFMOD*	m_pAudioDevice;

		//FMOD
		FMOD_SOUND*			m_pHandle;

		//Local
		const char*			m_pName;
		uint32				m_LengthMS;
	};
}
