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
		const char* pName	= "";
		const void* pData	= nullptr;
		uint32 DataSize		= 0;
	};

	class LAMBDA_API SoundEffect3D
	{
	public:
		DECL_REMOVE_COPY(SoundEffect3D);
		DECL_REMOVE_MOVE(SoundEffect3D);

		SoundEffect3D(const AudioDevice* pAudioDevice);
		~SoundEffect3D();

		bool Init(const SoundEffect3DDesc& desc);


		/**
			Call and forget, plays a single sound instance with the given properties
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
		uint32 m_LengthMS;
	};
}
