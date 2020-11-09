#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	struct SoundEffect2DDesc
	{
		String Filepath = "";
	};

	class ISoundEffect2D
	{
	public:
		DECL_INTERFACE(ISoundEffect2D);

		/*
		* Initialize this SoundEffect3D
		*	pDesc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool Init(const SoundEffect2DDesc* pDesc) = 0;

		/*
		* Play and forget, plays a single sound instance using this sound with the given properties
		*	volume - The volume that the sound should be played at in the range [-Inf, Inf]
		*	pitch - The pitch that the sound should be played at in the range [-Inf, Inf]
		*/
		virtual void PlayOnce(float volume = 1.0f, float pitch = 1.0f) = 0;

	};
}