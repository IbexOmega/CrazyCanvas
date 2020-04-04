#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class Time
	{
	public:
		DECL_STATIC_CLASS(Time);

		static void		PreInit()		{ }
		static uint64	GetPerformanceCounter()		{ return 0; }
		static uint64	GetPerformanceFrequency()	{ return 1; }
	};
}