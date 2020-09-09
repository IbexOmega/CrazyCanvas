#pragma once

#include "Time/API/Timestamp.h"

/*
	Profiler for both CPU and GPU
*/

namespace LambdaEngine
{
	class GPUProfiler;
	class CPUProfiler;

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