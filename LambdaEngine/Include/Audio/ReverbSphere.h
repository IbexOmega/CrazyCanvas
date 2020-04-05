#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

struct FMOD_REVERB3D;

namespace LambdaEngine
{
	class AudioDevice;

	struct ReverbSphereDesc
	{
		const char* pName		= "Reverb Sphere";
		glm::vec3 Position		= glm::vec3(0.0f);
		float MinDistance		= 1.0f;
		float MaxDistance		= 100.0f;
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

	private:
		//Engine
		const AudioDevice*	m_pAudioDevice;

		//FMOD
		FMOD_REVERB3D*		m_pReverb;

		//Locals
		const char*			m_pName;
	};
}