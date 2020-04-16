#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct SoundEffect3DDesc
	{
		const char* pFilepath	= "";
	};
	
	class ISoundEffect3D
	{
	public:
		DECL_INTERFACE(ISoundEffect3D);

				/*
		* Initialize this SoundEffect3DFMOD
		*	desc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool Init(const SoundEffect3DDesc& desc) = 0;

		/*
		* Play and forget, plays a single sound instance using this sound with the given properties
		*	position - The world space position, should be given in meters 
		*	velocity - The velocity that the sound instance is travelling at when emitted
		*	volume - The volume that the sound should be played at in the range [-Inf, Inf]
		*	pitch - The pitch that the sound should be played at in the range [-Inf, Inf]
		*/
		virtual void PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0.0f), float volume = 1.0f, float pitch = 1.0f) = 0;

	};
}