#pragma once

#include "LambdaEngine.h"
#include "AudioTypes.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class ISoundEffect2D;

	struct SoundInstance2DDesc
	{
		const char*		pName			= "SoundInstance3DFMOD";
		ISoundEffect2D* pSoundEffect	= nullptr;
		uint32			Flags			= FSoundModeFlags::SOUND_MODE_NONE;
		float			Volume			= 1.0f;
	};

	class ISoundInstance2D
	{
	public:
		DECL_INTERFACE(ISoundInstance2D);

		/*
		* Initialize this SoundInstance3D
		*	pDesc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool Init(const SoundInstance2DDesc* pDesc) = 0;

		/*
		* Play the sound instance
		*/
		virtual void Play() = 0;

		/*
		* Pause the sound instance
		*/
		virtual void Pause() = 0;

		/*
		* Stop the sound instance, releases some internal resources, a consecutive call to Play will restart the sound instance
		*/
		virtual void Stop() = 0;

		/*
		* Toggle the played/paused state of the sound instance
		*/
		virtual void Toggle() = 0;

		/*
		* Set the volume of the sound instance in the range [-Inf, Inf]
		*/
		virtual void SetVolume(float volume) = 0;

		/*
		* Set the pitch of the sound instance in the range [-Inf, Inf]
		*/
		virtual void SetPitch(float pitch) = 0;

		virtual float GetVolume()				const = 0;
		virtual float GetPitch()				const = 0;
	};
}