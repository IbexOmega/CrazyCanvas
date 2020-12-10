#pragma once

#include "Time/API/Timestamp.h"

#include "Debug/CPUProfiler.h"
#include "Debug/GPUProfiler.h"

#define PROFILING_ENABLED 0

#if PROFILING_ENABLED
	#define BEGIN_PROFILING_SEGMENT(_name_) LambdaEngine::Profiler::GetCPUProfiler()->BeginProfilingSegment(_name_)
	#define END_PROFILING_SEGMENT(_name_) LambdaEngine::Profiler::GetCPUProfiler()->EndProfilingSegment(_name_)
	#define PROFILE_FUNCTION(_name_, _func_call_) LambdaEngine::Profiler::GetCPUProfiler()->BeginProfilingSegment(_name_); _func_call_; LambdaEngine::Profiler::GetCPUProfiler()->EndProfilingSegment(_name_)
#else
	#define BEGIN_PROFILING_SEGMENT(_name_)
	#define END_PROFILING_SEGMENT(_name_)
	#define PROFILE_FUNCTION(_name_, _func_call_) _func_call_
#endif

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

		static void Tick(Timestamp delta);
		static void Render();

	private:
		static GPUProfiler* s_pGPUProfiler;
		static CPUProfiler* s_pCPUProfiler;
		static Timestamp s_Timestamp;
	};
}