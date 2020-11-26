#include "Audio/FMOD/ReverbSphereFMOD.h"
#include "Audio/FMOD/AudioDeviceFMOD.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ReverbSphereFMOD::ReverbSphereFMOD(const IAudioDevice* pAudioDevice) :
		m_pAudioDevice(reinterpret_cast<const AudioDeviceFMOD*>(pAudioDevice)),
		m_pReverb(nullptr)
	{
	}

	ReverbSphereFMOD::~ReverbSphereFMOD()
	{
		if (m_pReverb != nullptr)
		{
			if (FMOD_Reverb3D_Release(m_pReverb) != FMOD_OK)
			{
				LOG_WARNING("[ReverbSphereFMOD]: FMOD Reverb could not be released for %s", m_pName);
			}

			m_pReverb = nullptr;
		}
	}

	bool ReverbSphereFMOD::Init(const ReverbSphereDesc* pDesc)
	{
		VALIDATE(pDesc);

		m_pName = pDesc->pName;

		if (FMOD_System_CreateReverb3D(m_pAudioDevice->pSystem, &m_pReverb) != FMOD_OK)
		{
			LOG_WARNING("[ReverbSphereFMOD]: Reverb %s could not be created!", m_pName);
			return false;
		}

		SetReverbSetting(pDesc->ReverbSetting);

		Set3DAttributes(pDesc->Position, pDesc->MinDistance, pDesc->MaxDistance);

		LOG_DEBUG("[ReverbSphereFMOD]: Successfully initialized %s!", m_pName);

		return true;
	}

	void ReverbSphereFMOD::SetActive(bool active)
	{
		FMOD_Reverb3D_SetActive(m_pReverb, active ? 1 : 0);
	}

	void ReverbSphereFMOD::Set3DAttributes(const glm::vec3 position, float minDistance, float maxDistance)
	{
		FMOD_VECTOR fmodPosition = { position.x, position.y, position.z };

		FMOD_Reverb3D_Set3DAttributes(m_pReverb, &fmodPosition, minDistance, maxDistance);
	}

	void ReverbSphereFMOD::SetReverbSetting(EReverbSetting reverbSetting)
	{
		FMOD_REVERB_PROPERTIES reverbProperties = FMOD_PRESET_OFF;

		switch (reverbSetting)
		{
			case EReverbSetting::GENERIC:			reverbProperties = FMOD_PRESET_GENERIC; break;
			case EReverbSetting::PADDEDCELL:		reverbProperties = FMOD_PRESET_PADDEDCELL; break;
			case EReverbSetting::ROOM:				reverbProperties = FMOD_PRESET_ROOM; break;
			case EReverbSetting::BATHROOM:			reverbProperties = FMOD_PRESET_BATHROOM; break;
			case EReverbSetting::LIVINGROOM:		reverbProperties = FMOD_PRESET_LIVINGROOM; break;
			case EReverbSetting::STONEROOM:			reverbProperties = FMOD_PRESET_STONEROOM; break;
			case EReverbSetting::AUDITORIUM:		reverbProperties = FMOD_PRESET_AUDITORIUM; break;
			case EReverbSetting::CONCERTHALL:		reverbProperties = FMOD_PRESET_CONCERTHALL; break;
			case EReverbSetting::CAVE:				reverbProperties = FMOD_PRESET_CAVE; break;
			case EReverbSetting::ARENA:				reverbProperties = FMOD_PRESET_ARENA; break;
			case EReverbSetting::HANGAR:			reverbProperties = FMOD_PRESET_HANGAR; break;
			case EReverbSetting::CARPETTEDHALLWAY:	reverbProperties = FMOD_PRESET_CARPETTEDHALLWAY; break;
			case EReverbSetting::HALLWAY:			reverbProperties = FMOD_PRESET_HALLWAY; break;
			case EReverbSetting::STONECORRIDOR:		reverbProperties = FMOD_PRESET_STONECORRIDOR; break;
			case EReverbSetting::ALLEY:				reverbProperties = FMOD_PRESET_ALLEY; break;
			case EReverbSetting::FOREST:			reverbProperties = FMOD_PRESET_FOREST; break;
			case EReverbSetting::CITY:				reverbProperties = FMOD_PRESET_CITY; break;
			case EReverbSetting::MOUNTAINS:			reverbProperties = FMOD_PRESET_MOUNTAINS; break;
			case EReverbSetting::QUARRY:			reverbProperties = FMOD_PRESET_QUARRY; break;
			case EReverbSetting::PLAIN:				reverbProperties = FMOD_PRESET_PLAIN; break;
			case EReverbSetting::PARKINGLOT:		reverbProperties = FMOD_PRESET_PARKINGLOT; break;
			case EReverbSetting::SEWERPIPE:			reverbProperties = FMOD_PRESET_SEWERPIPE; break;
			case EReverbSetting::UNDERWATER:		reverbProperties = FMOD_PRESET_UNDERWATER; break;
		}

		FMOD_Reverb3D_SetProperties(m_pReverb, &reverbProperties);
	}
}