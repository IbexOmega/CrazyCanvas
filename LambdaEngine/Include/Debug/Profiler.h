#pragma once

#include "Time/API/Timestamp.h"

#include "Debug/CPUProfiler.h"
#include "Debug/GPUProfiler.h"

/*
	Profiler for both CPU and GPU
*/

namespace LambdaEngine
{
	class Profiler
	{
	public:
		Profiler();
		~Profiler() = default;

	public:
		static CPUProfiler* GetCPUProfiler();
		static GPUProfiler* GetGPUProfiler();

		static void Render(Timestamp delta);

	private:
		static GPUProfiler* s_pGPUProfiler;
		static CPUProfiler* s_pCPUProfiler;
	};
}