#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct MusicDesc
	{
		const char*		pFilepath		= "";
		float32			Volume			= 1.0f;
		float32			Pitch			= 1.0f;
	};

	class IMusic
	{
	public:
		DECL_INTERFACE(IMusic);

		/*
		* Initialize this Music
		*	pDesc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool Init(const MusicDesc* pDesc) = 0;

		/*
		* Play the music
		*/
		virtual void Play() = 0;

		/*
		* Pause the music
		*/
		virtual void Pause() = 0;

		/*
		* Toggle the played/paused state of the music
		*/
		virtual void Toggle() = 0;

		/*
		* Set the volume of the music in the range [-Inf, Inf]
		*/
		virtual void SetVolume(float volume) = 0;

		/*
		* Set the pitch of the music in the range [-Inf, Inf]
		*/
		virtual void SetPitch(float pitch) = 0;

		virtual float GetVolume()				const = 0;
		virtual float GetPitch()				const = 0;

	};
}