#pragma once

#include "FMOD.h"
#include "Audio/API/IReverbSphere.h"

namespace LambdaEngine
{
	class IAudioDevice;
	class AudioDeviceFMOD;

	class ReverbSphereFMOD : public IReverbSphere
	{
	public:
		ReverbSphereFMOD(const IAudioDevice* pAudioDevice);
		~ReverbSphereFMOD();

		virtual bool Init(const ReverbSphereDesc* pDesc) override final;
		virtual void SetActive(bool active) override final;
		virtual void Set3DAttributes(const glm::vec3 position, float minDistance, float maxDistance) override final;
		virtual void SetReverbSetting(EReverbSetting reverbSetting) override final;
		
	private:
		//Engine
		const AudioDeviceFMOD*	m_pAudioDevice;

		//FMOD
		FMOD_REVERB3D*		m_pReverb;

		//Locals
		const char*			m_pName;
	};
}