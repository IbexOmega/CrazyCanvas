#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class Time
	{
	public:
		DECL_STATIC_CLASS(Time);

		static void	PreInit() { }
		
		/*
		* Returns a platform-specific number of ticks (Not the same as CPU- cycles). The counter is only really useful together 
		* with the frequency that can be queried from PlatformTime::GetPerformanceFrequency. This means that it is not necessary 
		* what the tick count represents as long as PlatformTime::GetPerformanceCounter() / PlatformTime::GetPerformanceFrequency() will 
		* result in the current time.
		*
		* return - The number of ticks
		*/
		static uint64	GetPerformanceCounter()		{ return 0; }
		static uint64	GetPerformanceFrequency()	{ return 1; }
	};
}