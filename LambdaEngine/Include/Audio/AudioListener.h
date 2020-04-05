#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class AudioDevice;

	struct AudioListenerDesc
	{
		const char* pName		= "AudioListener";
		uint32 ListenerIndex	= 0;
	};

	class LAMBDA_API AudioListener
	{
	public:
		DECL_REMOVE_COPY(AudioListener);
		DECL_REMOVE_MOVE(AudioListener);

		AudioListener(const AudioDevice* pAudioDevice);
		~AudioListener();

		/*
		* Initialize this AudioListener
		*	desc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		bool Init(const AudioListenerDesc& desc);

		/*
		* Update the 3D Attributes of this AudioListener
		*	position - The world space position, should be given in meters
		*	forward - The forward facing vector, assumed to be normalized
		*	up - The upwards pointing vector, assumed to be normalized
		*/
		void Update(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

	private:
		//Engine
		const AudioDevice*	m_pAudioDevice;

		//Locals
		const char*			m_pName;
		uint32				m_ListenerIndex;
	};
}