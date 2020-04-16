#pragma once

#include "FMOD.h"
#include "Audio/API/IAudioListener.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class AudioListenerFMOD : public IAudioListener
	{
	public:
		AudioListenerFMOD(const IAudioDevice* pAudioDevice);
		~AudioListenerFMOD();

		virtual bool Init(const AudioListenerDesc& desc) override final;
		virtual void Update(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) override final;
		
	private:
		//Engine
		const AudioDeviceFMOD*	m_pAudioDevice;

		//Locals
		const char*			m_pName;
		uint32				m_ListenerIndex;
	};
}