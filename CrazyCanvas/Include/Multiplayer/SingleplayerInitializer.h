#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class SingleplayerInitializer
	{
	public:
		DECL_STATIC_CLASS(SingleplayerInitializer);

		static void Init();
		static void Release();

		static void Setup();

	};
}