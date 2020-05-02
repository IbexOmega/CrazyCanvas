#pragma once

#include "LambdaEngine.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	struct AudioListenerDesc
	{
		const char* pName		= "AudioListenerFMOD";
		uint32 ListenerIndex	= 0;
	};
	
	class IAudioListener
	{
	public:
		DECL_INTERFACE(IAudioListener);

		/*
		* Initialize this AudioListenerFMOD
		*	pDesc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool Init(const AudioListenerDesc* pDesc) = 0;

		/*
		* Update the 3D Attributes of this AudioListenerFMOD
		*	position - The world space position, should be given in meters
		*	forward - The forward facing vector, assumed to be normalized
		*	up - The upwards pointing vector, assumed to be normalized
		*/
		virtual void Update(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) = 0;
	};
}