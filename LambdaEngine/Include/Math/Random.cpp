#include "Random.h"

#include <time.h>
#include <chrono>

namespace LambdaEngine
{
	std::default_random_engine Random::s_Generator(std::chrono::system_clock::now().time_since_epoch().count());

	int32 Random::Int32(int32 min, int32 max)
	{
		std::uniform_int_distribution<int32> dist(min, max);
		return dist(s_Generator);
	}

	float32 Random::Float32(float32 min, float32 max)
	{
		std::uniform_real_distribution<float32> dist(min, max);
		return dist(s_Generator);
	}

	float32 Random::Float32()
	{
		std::uniform_real_distribution<float32> dist(0, 1.0F);
		return dist(s_Generator);
	}

	uint64 Random::UInt64()
	{
		std::uniform_int_distribution<uint64> dist(0, UINT64_MAX);
		return dist(s_Generator);
	}

	bool Random::Bool()
	{
		return Int32(0, 2);
	}
}