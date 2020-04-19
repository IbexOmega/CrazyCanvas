#pragma once

#include "LambdaEngine.h"
#include <random>

namespace LambdaEngine
{
	class LAMBDA_API Random
	{
		friend class EngineLoop;

	public:
		DECL_STATIC_CLASS(Random);

		static int32   Int32(int32 min, int32 max);
		static float32 Float32(float32 min, float32 max);

		static uint64  UInt64();
		static float32 Float32();
		static bool    Bool();

	private:
		static std::default_random_engine s_Generator;
	};
}