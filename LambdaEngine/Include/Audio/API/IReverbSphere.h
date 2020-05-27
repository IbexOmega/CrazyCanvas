#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
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
	
	class IReverbSphere
	{
	public:
		DECL_INTERFACE(IReverbSphere);

		/*
		* Initialize this ReverbSphereFMOD
		*	pDesc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool Init(const ReverbSphereDesc* pDesc) = 0;

		/*
		* Set whether the geometry should be processed by the audio engine
		*/
		virtual void SetActive(bool active) = 0;

		/*
		* Set the 3D attributes of this ReverbSphereFMOD
		*	position - The world position
		*	minDistance - The distance from the centerpoint within which the reverb will have full effect
		*	maxDistance - The distance from the centerpoint within which the reverb will have no effect
		*/
		virtual void Set3DAttributes(const glm::vec3 position, float minDistance, float maxDistance) = 0;

		/*
		* Set the reverb environment setting
		*/
		virtual void SetReverbSetting(EReverbSetting reverbSetting) = 0;
	};
}