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

		static void PreInit();

		static int32   Int32(int32 min, int32 max);
		static float32 Float32(float32 min, float32 max);

		static uint64  UInt64();
		static float32 Float32();
		static bool    Bool();

	private:
		static std::default_random_engine s_Generator;
	};
}

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(pop)
#endif
