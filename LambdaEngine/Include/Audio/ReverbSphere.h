#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

struct FMOD_REVERB3D;

namespace LambdaEngine
{
	class AudioDevice;

	enum class EReverbSetting : uint32
	{
		GENERIC				= 0,
		PADDEDCELL			= 1,
		ROOM				= 2,
		BATHROOM			= 3,
		LIVINGROOM			= 4,
		STONEROOM			= 5,
		AUDITORIUM			= 6,
		CONCERTHALL			= 7,
		CAVE				= 8,
		ARENA				= 9,
		HANGAR				= 10,
		CARPETTEDHALLWAY	= 11,
		HALLWAY				= 12,
		STONECORRIDOR		= 13,
		ALLEY				= 14,
		FOREST				= 15,
		CITY				= 16,
		MOUNTAINS			= 17,
		QUARRY				= 18,
		PLAIN				= 19,
		PARKINGLOT			= 20,
		SEWERPIPE			= 21,
		UNDERWATER			= 22
	};

	struct ReverbSphereDesc
	{
		const char* pName				= "Reverb Sphere";
		glm::vec3 Position				= glm::vec3(0.0f);
		float MinDistance				= 5.0f;
		float MaxDistance				= 10.0f;
		EReverbSetting ReverbSetting	= EReverbSetting::GENERIC;
	};

	class LAMBDA_API ReverbSphere
	{
	public:
		DECL_REMOVE_COPY(ReverbSphere);
		DECL_REMOVE_MOVE(ReverbSphere);

		ReverbSphere(const AudioDevice* pAudioDevice);
		~ReverbSphere();

		/*
		* Initialize this ReverbSphere
		*	desc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		bool Init(const ReverbSphereDesc& desc);

		/*
		* Set whether the geometry should be processed by the audio engine
		*/
		void SetActive(bool active);

		/*
		* Set the 3D attributes of this ReverbSphere
		*	position - The world position
		*	minDistance - The distance from the centerpoint within which the reverb will have full effect
		*	maxDistance - The distance from the centerpoint within which the reverb will have no effect
		*/
		void Set3DAttributes(const glm::vec3 position, float minDistance, float maxDistance);

		/*
		* Set the reverb environment setting
		*/
		void SetReverbSetting(EReverbSetting reverbSetting);

	private:
		//Engine
		const AudioDevice*	m_pAudioDevice;

		//FMOD
		FMOD_REVERB3D*		m_pReverb;

		//Locals
		const char*			m_pName;
	};
}