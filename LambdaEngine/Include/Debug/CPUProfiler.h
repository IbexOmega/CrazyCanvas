#pragma once

#include "Input/API/InputCodes.h"
#include "Time/API/Timestamp.h"
#include "Time/API/Clock.h"
#include "Application/API/Application.h"

#include <chrono>
#include <string>
#include <fstream>
#include <mutex>

#ifdef LAMBDA_DEBUG
#define LAMBDA_PROFILER_BEGIN_SESSION(name, fileName) LambdaEngine::CPUProfiler::Get()->BeginSession(name, fileName)
#define LAMBDA_PROFILER_END_SESSION() LambdaEngine::CPUProfiler::Get()->EndSession()
#define LAMBDA_PROFILER_SCOPE(name) LambdaEngine::InstrumentationTimer instrumentationTimer##__LINE__(name)
#define LAMBDA_PROFILER_FUNCTION() LAMBDA_PROFILER_SCOPE(__FUNCTION__ )

#define LAMBDA_PROFILER_SAMPLE_BEGIN_SESSION(name, fileName) { if(!LambdaEngine::CPUProfiler::g_RunProfilingSample) { LambdaEngine::CPUProfiler::Get()->BeginSession(name, fileName); LambdaEngine::CPUProfiler::g_RunProfilingSample = true; }}
#define LAMBDA_PROFILER_SAMPLE_END_SESSION() { if(LambdaEngine::CPUProfiler::g_RunProfilingSample) { LambdaEngine::CPUProfiler::Get()->EndSession(); LambdaEngine::CPUProfiler::g_RunProfilingSample = false; }}
#define LAMBDA_PROFILER_SAMPLE_SCOPE(name) LambdaEngine::InstrumentationTimer instrumentationTimerRendering##__LINE__(name, LambdaEngine::CPUProfiler::g_RunProfilingSample)
#define LAMBDA_PROFILER_SAMPLE_FUNCTION() LAMBDA_PROFILER_SAMPLE_SCOPE(__FUNCTION__ )

#define LAMBDA_PROFILER_TOGGLE_SAMPLE(key, frameCount) CPUProfiler::Get().ToggleSample(key, frameCount)
#define LAMBDA_PROFILER_TOGGLE_SAMPLE_POOL(pool, key, frameCount) CPUProfiler::Get().ToggleSample(pool, key, frameCount)

#else
#define LAMBDA_PROFILER_BEGIN_SESSION(name, fileName)
#define LAMBDA_PROFILER_END_SESSION()
#define LAMBDA_PROFILER_SCOPE(name)
#define LAMBDA_PROFILER_FUNCTION()

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
		int64_t m_StartTime;
		std::string m_Name;
		bool m_Active;
	};

	class CPUProfiler
	{
		struct FinishedProfilingSegment
		{
			String Name;
			float64 DeltaTime;
			std::multiset<FinishedProfilingSegment> ChildProfilingSegments;

			inline bool operator<(const FinishedProfilingSegment& profilingSegment) const
			{
				return DeltaTime < profilingSegment.DeltaTime;
			}
		};

		struct LiveProfilingSegment
		{
			Clock Clock;
			std::multiset<FinishedProfilingSegment> ChildProfilingSegments;
		};

		struct ProfilingTick
		{
			THashTable<String, uint32> ProfilingSegmentsMap;
			TArray<LiveProfilingSegment> LiveProfilingSegments;
			std::multiset<FinishedProfilingSegment> FinishedProfilingSegments;
			TArray<uint32> ProfilingSegmentStack;
			float64 TotalDeltaTime = 0;

			inline void Reset()
			{
				ProfilingSegmentsMap.clear();
				LiveProfilingSegments.Clear();
				FinishedProfilingSegments.clear();
				TotalDeltaTime = 0;
			}
		};

	public:
		struct ProfileData
		{
			std::string Name;
			uint64_t Start, End;
			size_t TID = 0;
			size_t PID = 0;
		};
		CPUProfiler();
		~CPUProfiler();

		static CPUProfiler* Get();

		void BeginProfilingSegment(const String& name);
		void EndProfilingSegment(const String& name);

		void BeginSession(const std::string& name, const std::string& filePath = "CPUResult.json");

		void Write(ProfileData data);

		void SetStartTime(uint64_t time);

		void ToggleSample(EKey key, uint32_t frameCount);

		void EndSession();

		void Tick(Timestamp delta);

		void Render();


		static bool g_RunProfilingSample;

	private:
		void RenderFinishedProfilingSegment(const FinishedProfilingSegment& profilingSegment, float64 parentDeltaTime);

	private:
		std::mutex m_Mutex;
		std::ofstream m_File;
		unsigned long long m_Counter;
		uint64_t m_StartTime = 0;
		float64 m_TimeSinceUpdate = 0.0f;
		CPUStatistics m_CPUStat = {};
		float32 m_UpdateFrequency = 1.0f;
		Timestamp m_Timestamp = 0;

		SpinLock m_ProfilingSegmentSpinlock;
		ProfilingTick m_ProfilingTicks[2];	
		uint32 m_PreviousProfilingTick = 0;
		uint32 m_CurrentProfilingTick = 1;
	};
}