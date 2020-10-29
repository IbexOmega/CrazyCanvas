#pragma once
#include "LambdaEngine.h"


#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(push)
	#pragma warning(disable : 4251)
#endif

#include <random>

namespace LambdaEngine
{
	class LAMBDA_API Random
	{
	public:
		DECL_STATIC_CLASS(Random);

		static int32   Int32(int32 min = INT32_MIN, int32 max = INT32_MAX);
		static uint32  UInt32(uint32 min = 0, uint32 max = UINT32_MAX);
		static float32 Float32(float32 min, float32 max);

		static uint64  UInt64(uint64 min = 0, uint64 max = UINT64_MAX);
		static float32 Float32();
		static bool    Bool();

	private:
		static std::default_random_engine s_Generator;
	};
}

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(pop)
#endif
