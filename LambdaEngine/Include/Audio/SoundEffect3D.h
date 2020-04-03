#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

#include "SoundHelper.h"

struct FMOD_SOUND;

namespace LambdaEngine
{
	class AudioDevice;
	class SoundInstance;

	class LAMBDA_API SoundEffect3D
	{
	public:
		DECL_REMOVE_COPY(SoundEffect3D);
		DECL_REMOVE_MOVE(SoundEffect3D);

		SoundEffect3D(const AudioDevice* pAudioDevice);
		~SoundEffect3D();

		bool Init(const SoundDesc& desc);

		void PlayAt(const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0.0f), float volume = 1.0f, float pitch = 1.0f);

	private:
		const AudioDevice* m_pAudioDevice;

		FMOD_SOUND*			m_pHandle;
	};
}
