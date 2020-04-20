#include "Log/Log.h"

#include "Audio/Lambda/SoundEffect3DLambda.h"

namespace LambdaEngine
{
	SoundEffect3DLambda::SoundEffect3DLambda(const IAudioDevice* pAudioDevice)
	{
		UNREFERENCED_VARIABLE(pAudioDevice);
	}

	SoundEffect3DLambda::~SoundEffect3DLambda()
	{
		
	}

	bool SoundEffect3DLambda::Init(const SoundEffect3DDesc& desc)
	{
		LoadWavFile(desc.pFilepath, &m_pWaveForm, &m_Header);
		
		/*constexpr float AMPLITUDE = 0.5f;
		constexpr float FREQUENCY = 1000;
		constexpr float PHASE = 0.0f;
		constexpr uint32 SAMPLE_RATE = 44100;
		
		for (uint32 i = 0; i < m_SampleCount; i++)
		{
			m_pWaveForm[i] = AMPLITUDE * glm::cos(glm::two_pi<float>() * (FREQUENCY * i / SAMPLE_RATE + PHASE));
		}*/
		
		return true;
	}

	void SoundEffect3DLambda::PlayOnceAt(const glm::vec3& position, const glm::vec3& velocity, float volume, float pitch)
	{
		UNREFERENCED_VARIABLE(position);
		UNREFERENCED_VARIABLE(velocity);
		UNREFERENCED_VARIABLE(volume);
		UNREFERENCED_VARIABLE(pitch);
	}
}
