#pragma once

#include "LambdaEngine.h"
#include "Math/Math.h"

struct FMOD_SOUND;
struct FMOD_CHANNEL;
struct FMOD_CHANNELGROUP;

namespace LambdaEngine
{
	class AudioDevice;

	enum ESoundFlags : uint32
	{
		NONE			= BIT(0),
		LOOPING			= BIT(1),
	};

	struct SoundDesc
	{
		const char* pName		= "";
		const void* pData		= nullptr;
		uint32 DataSize			= 0;

		ESoundFlags Flags		= ESoundFlags::NONE;
	};

	class LAMBDA_API Sound
	{
	public:
		DECL_REMOVE_COPY(Sound);
		DECL_REMOVE_MOVE(Sound);

		Sound(const AudioDevice* pAudioDevice);
		~Sound();

		bool Init(const SoundDesc& desc);

		void Toggle();

		void SetPanning(float panning);
		void SetVolume(float volume);
		void SetPitch(float pitch);

		float GetPanning();
		float GetVolume();
		float GetPitch();

	private:
		const AudioDevice* m_pAudioDevice;

		float m_Panning;

		FMOD_SOUND*			m_pHandle;
		FMOD_CHANNEL*		m_pChannel;
		uint32				m_NumChannels;
	};
}
