#pragma once

#include "Input/API/InputCodes.h"
#include "Time/API/Timestamp.h"

#include <chrono>
#include <string>
#include <fstream>
#include <mutex>

#ifdef LAMBDA_DEBUG
#define LAMBDA_PROFILER_BEGIN_SESSION(name, fileName) Instrumentation::get().beginSession(name, fileName)
#define LAMBDA_PROFILER_END_SESSION() Instrumentation::get().endSession()
#define LAMBDA_PROFILER_SCOPE(name) InstrumentationTimer instrumentationTimer##__LINE__(name)
#define LAMBDA_PROFILER_FUNCTION() LAMBDA_PROFILER_SCOPE(__FUNCTION__ )

#define LAMBDA_PROFILER_SAMPLE_BEGIN_SESSION(name, fileName) {if(!Instrumentation::g_runProfilingSample) { Instrumentation::get().beginSession(name, fileName); Instrumentation::g_runProfilingSample = true; }}
#define LAMBDA_PROFILER_SAMPLE_END_SESSION() {if(Instrumentation::g_runProfilingSample) { Instrumentation::get().endSession(); Instrumentation::g_runProfilingSample = false; } }
#define LAMBDA_PROFILER_SAMPLE_SCOPE(name) InstrumentationTimer instrumentationTimerRendering##__LINE__(name, Instrumentation::g_runProfilingSample)
#define LAMBDA_PROFILER_SAMPLE_FUNCTION() LAMBDA_PROFILER_SAMPLE_SCOPE(__FUNCTION__ )

#define LAMBDA_PROFILER_TOGGLE_SAMPLE(key, frameCount) Instrumentation::get().toggleSample(key, frameCount)
#define LAMBDA_PROFILER_TOGGLE_SAMPLE_POOL(pool, key, frameCount) Instrumentation::get().toggleSample(pool, key, frameCount)
#define LAMBDA_PROFILER_WRITE_VULKAN_DATA() Instrumentation::get().writeVulkanData()

#else
#define LAMBDRA_PROFILER_BEGIN_SESSION(name, fileName)
#define LAMBDRA_PROFILER_END_SESSION()
#define LAMBDRA_PROFILER_SCOPE(name)
#define LAMBDRA_PROFILER_FUNCTION()

#define LAMBDA_PROFILER_SAMPLE_BEGIN_SESSION(name, fileName)
#define LAMBDA_PROFILER_SAMPLE_END_SESSION()
#define LAMBDA_PROFILER_SAMPLE_SCOPE(name)
#define LAMBDA_PROFILER_SAMPLE_FUNCTION()

#define LAMBDA_PROFILER_TOGGLE_SAMPLE(key, frameCount)
#define LAMBDA_PROFILER_TOGGLE_SAMPLE_POOL(pool, key, frameCount)
#define LAMBDA_PROFILER_WRITE_VULKAN_DATA()
#endif

namespace LambdaEngine
{
	class InstrumentationTimer
	{
	public:
		InstrumentationTimer(const std::string& name, bool active = true);
		~InstrumentationTimer();

		void Start();
		void Stop();

	private:
		int64_t startTime;
		std::string name;
		bool active;
	};

	class CPUProfiler
	{
	public:
		struct ProfileData
		{
			std::string name;
			uint64_t start, end;
			size_t tid = 0;
			size_t pid = 0;
		};
		CPUProfiler();
		~CPUProfiler();

		static CPUProfiler* Get();

		void BeginSession(const std::string& name, const std::string& filePath = "CPUResult.json");

		void Write(ProfileData data);

		void SetStartTime(uint64_t time);

		void ToggleSample(EKey key, uint32_t frameCount);

		void EndSession();

		void Render(Timestamp delta);


		static bool g_runProfilingSample;

	private:
		std::mutex mutex;
		std::ofstream file;
		unsigned long long counter;
		uint64_t startTime = 0;
	};
}