#pragma once

#include "FMOD.h"
#include "Audio/API/ISoundEffect2D.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class SoundEffect2DFMOD : public ISoundEffect2D
	{
	public:
		SoundEffect2DFMOD(const IAudioDevice* pAudioDevice);
		~SoundEffect2DFMOD();

		virtual bool Init(const SoundEffect2DDesc* pDesc) override final;
		virtual void PlayOnce(float volume, float pitch) override final;

		FMOD_SOUND* GetHandle();
		uint32 GetLengthMS();

	private:
		//Engine
		const AudioDeviceFMOD* m_pAudioDevice;

		//FMOD
		FMOD_SOUND* m_pHandle;

		//Local
		String				m_Name;
		uint32				m_LengthMS;
	};
}
