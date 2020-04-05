#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "SoundHelper.h"

struct FMOD_SOUND;
struct FMOD_CHANNEL;

namespace LambdaEngine
{
	class AudioDevice;

	struct SoundEffect3DDesc
	{
		const char* pFilepath	= "";
		uint32 DataSize			= 0;
	};

	class LAMBDA_API SoundEffect3D
	{
	public:
		DECL_REMOVE_COPY(SoundEffect3D);
		DECL_REMOVE_MOVE(SoundEffect3D);

		SoundEffect3D(const AudioDevice* pAudioDevice);
		~SoundEffect3D();

		/*
		* Initialize this SoundEffect3D
		*	desc - A description of initialization parameters
		* return - true if the initialization was successfull, otherwise returns false
		*/
		bool Init(const SoundEffect3DDesc& desc);

		/*
		* Play and forget, plays a single sound instance using this sound with the given properties
		*	position - The world space position, should be given in meters 
		*	velocity - The velocity that the sound instance is travelling at when emitted
		*	volume - The volume that the sound should be played at in the range [-Inf, Inf]
		*	pitch - The pitch that the sound should be played at in the range [-Inf, Inf]
		*/
		void PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0.0f), float volume = 1.0f, float pitch = 1.0f);

		FMOD_SOUND* GetHandle()		{ return m_pHandle; }
		uint32 GetLengthMS()		{ return m_LengthMS; }

	private:
		//Engine
		const AudioDevice*	m_pAudioDevice;

		//FMOD
		FMOD_SOUND*			m_pHandle;

		//Local
		uint32				m_LengthMS;
	};
}
