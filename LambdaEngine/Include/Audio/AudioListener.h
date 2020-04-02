#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	class AudioDevice;

	struct AudioListenerDesc
	{
		uint32 ListenerIndex = 0;
	};

	class LAMBDA_API AudioListener
	{
	public:
		DECL_REMOVE_COPY(AudioListener);
		DECL_REMOVE_MOVE(AudioListener);

		AudioListener(const AudioDevice* pAudioDevice);
		~AudioListener();

		void Init(const AudioListenerDesc& desc);

		void Update(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));

	private:
		const AudioDevice* m_pAudioDevice;

		uint32 m_ListenerIndex;
	};
}