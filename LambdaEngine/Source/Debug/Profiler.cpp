#include "PreCompiled.h"
#include "Debug/Profiler.h"
#include "Debug/CPUProfiler.h"
#include "Debug/GPUProfiler.h"

#include <imgui.h>

namespace LambdaEngine
{
	GPUProfiler* Profiler::s_pGPUProfiler 	= nullptr;
	CPUProfiler* Profiler::s_pCPUProfiler 	= nullptr;
	Timestamp Profiler::s_Timestamp			= 0;

	Profiler::Profiler()
	{
	}

	CPUProfiler* Profiler::GetCPUProfiler()
	{
		if (s_pCPUProfiler == nullptr)
			s_pCPUProfiler = CPUProfiler::Get();
		return s_pCPUProfiler;
	}

	GPUProfiler* Profiler::GetGPUProfiler()
	{
		if (s_pGPUProfiler == nullptr)
			s_pGPUProfiler = GPUProfiler::Get();
		return s_pGPUProfiler;
	}

	void Profiler::Tick(Timestamp delta)
	{
		s_Timestamp = delta;
		GetGPUProfiler()->Tick(delta);
	}

	void Profiler::Render()
	{
		ImGui::Begin("Profiling data");

		GetCPUProfiler()->Render(s_Timestamp);
		GetGPUProfiler()->Render();

		ImGui::End();
	}

}