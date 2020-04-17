#pragma once

#include "LambdaEngine.h"
#include "AudioTypes.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class ISoundEffect3D;
	
	struct SoundInstance3DDesc
	{
		const char* pName					= "SoundInstance3DFMOD";
		ISoundEffect3D* pSoundEffect		= nullptr;
		uint32 Flags						= FSoundModeFlags::SOUND_MODE_NONE;
	};
	
	class ISoundInstance3D
	{
	public:
		DECL_INTERFACE(ISoundInstance3D);

		/*
		* Initialize this SoundInstance3DFMOD
		*	desc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		virtual bool Init(const SoundInstance3DDesc& desc) = 0;

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
		* Set the world position of the sound instance
		*	position - The world space position, should be given in meters
		*/
		virtual void SetPosition(const glm::vec3& position) = 0;

		/*
		* Set the volume of the sound instance in the range [-Inf, Inf]
		*/
		virtual void SetVolume(float volume) = 0;

		/*
		* Set the pitch of the sound instance in the range [-Inf, Inf]
		*/
		virtual void SetPitch(float pitch) = 0;

		virtual const glm::vec3& GetPosition() = 0;
		virtual float GetVolume() = 0;
		virtual float GetPitch() = 0;
	};
}