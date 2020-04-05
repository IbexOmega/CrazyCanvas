#include "Audio/ReverbSphere.h"
#include "Audio/AudioDevice.h"
#include "Audio/Audio.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ReverbSphere::ReverbSphere(const AudioDevice* pAudioDevice) : 
		m_pAudioDevice(pAudioDevice),
		m_pReverb(nullptr)
	{
	}

	ReverbSphere::~ReverbSphere()
	{
		if (m_pReverb != nullptr)
		{
			if (FMOD_Reverb3D_Release(m_pReverb) != FMOD_OK)
			{
				LOG_WARNING("[ReverbSphere]: FMOD Reverb could not be released for %s", m_pName);
			}

			m_pReverb = nullptr;
		}
	}

	bool ReverbSphere::Init(const ReverbSphereDesc& desc)
	{
		m_pName = desc.pName;

		if (FMOD_System_CreateReverb3D(m_pAudioDevice->pSystem, &m_pReverb) != FMOD_OK)
		{
			LOG_WARNING("[ReverbSphere]: Reverb %s could not be created!", m_pName);
			return false;
		}

		FMOD_REVERB_PROPERTIES reverbProperties = FMOD_PRESET_HANGAR;

		if (FMOD_Reverb3D_SetProperties(m_pReverb, &reverbProperties) != FMOD_OK)
		{
			LOG_WARNING("[ReverbSphere]: Reverb %s could not be initialized!", m_pName);
			return false;
		}

		FMOD_VECTOR fmodPosition = { desc.Position.x, desc.Position.y, desc.Position.z };

		if (FMOD_Reverb3D_Set3DAttributes(m_pReverb, &fmodPosition, desc.MinDistance, desc.MaxDistance) != FMOD_OK)
		{
			LOG_WARNING("[ReverbSphere]: Reverb %s 3D Attributes could not be initialized!", m_pName);
			return false;
		}

		D_LOG_MESSAGE("[ReverbSphere]: Successfully initialized %s!", m_pName);

		return true;
	}

	void ReverbSphere::SetActive(bool active)
	{
		FMOD_Reverb3D_SetActive(m_pReverb, active ? 1 : 0);
	}
}