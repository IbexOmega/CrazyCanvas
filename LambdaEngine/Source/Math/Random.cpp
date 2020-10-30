#include "Math/Random.h"

#include "Time/API/PlatformTime.h"

namespace LambdaEngine
{
	std::default_random_engine Random::s_Generator(0);

	void Random::PreInit()
	{
		s_Generator = std::default_random_engine((uint32)PlatformTime::GetPerformanceCounter());
	}

	int32 Random::Int32(int32 min, int32 max)
	{
		std::uniform_int_distribution<int32> dist(min, max);
		return dist(s_Generator);
	}

	uint32 Random::UInt32(uint32 min, uint32 max)
	{
		std::uniform_int_distribution<uint32> dist(min, max);
		return dist(s_Generator);
	}

	float32 Random::Float32(float32 min, float32 max)
	{
		std::uniform_real_distribution<float32> dist(min, max);
		return dist(s_Generator);
	}

	float32 Random::Float32()
	{
		std::uniform_real_distribution<float32> dist(0.0F, 1.0F);
		return dist(s_Generator);
	}

	uint64 Random::UInt64(uint64 min, uint64 max)
	{
		std::uniform_int_distribution<uint64> dist(min, max);
		return dist(s_Generator);
	}

	bool Random::Bool()
	{
		return Int32(0, 2);
	}
}