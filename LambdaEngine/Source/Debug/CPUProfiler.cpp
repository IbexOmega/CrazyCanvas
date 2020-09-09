#include "PreCompiled.h"
#include "Debug/CPUProfiler.h"
#include "Input/API/Input.h"

#include <imgui.h>

// Code modifed from: https://github.com/TheCherno/Hazel

/*
	InstrumentationTimer class
*/

namespace LambdaEngine
{

	InstrumentationTimer::InstrumentationTimer(const std::string& name, bool active) : name(name), active(active)
	{
		Start();
	}

	InstrumentationTimer::~InstrumentationTimer()
	{
		Stop();
	}

	void InstrumentationTimer::Start()
	{
		if (this->active)
			this->startTime = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
	}

	void InstrumentationTimer::Stop()
	{
		if (this->active)
		{
			long long start = this->startTime;
			long long end = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();

			size_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
			CPUProfiler::Get()->Write({ this->name, (uint64_t)start, (uint64_t)end, tid });
		}
	}

	/*
		Instrumentation class
	*/

	bool CPUProfiler::g_runProfilingSample = false;

	CPUProfiler::CPUProfiler() : counter(0)
	{
	}

	CPUProfiler::~CPUProfiler()
	{
	}

	CPUProfiler* CPUProfiler::Get()
	{
		static CPUProfiler cpuProfiler;
		return &cpuProfiler;
	}

	void CPUProfiler::BeginSession(const std::string& name, const std::string& filePath)
	{
		this->counter = 0;
		this->file.open(filePath);
		this->file << "{\"otherData\": {}, \"displayTimeUnit\": \"ms\", \"traceEvents\": [";
		this->file.flush();
	}

	void CPUProfiler::Write(ProfileData data)
	{
		if (data.pid == 0) {
			data.start -= this->startTime;
			data.end -= this->startTime;
		}

		std::lock_guard<std::mutex> lock(this->mutex);
		if (this->counter++ > 0) this->file << ",";

		std::string name = data.name;
		std::replace(name.begin(), name.end(), '"', '\'');

		this->file << "\n{";
		this->file << "\"name\": \"" << name << "\",";
		this->file << "\"cat\": \"function\",";
		this->file << "\"ph\": \"X\",";
		this->file << "\"pid\": " << data.pid << ",";
		this->file << "\"tid\": " << data.tid << ",";
		this->file << "\"ts\": " << data.start << ",";
		this->file << "\"dur\": " << (data.end - data.start);
		this->file << "}";

		this->file.flush();
	}

	void CPUProfiler::SetStartTime(uint64_t time)
	{
		this->startTime = time;
	}

	void CPUProfiler::ToggleSample(EKey key, uint32_t frameCount)
	{
		static uint32_t frameCounter = 0;
		if (Input::GetKeyboardState().IsKeyDown(key))
		{
			frameCounter = 0;
			CPUProfiler::g_runProfilingSample = true;
			D_LOG_INFO("Start Profiling");
		}

		if (CPUProfiler::g_runProfilingSample)
		{
			frameCounter++;
			if (frameCounter > frameCount)
			{
				CPUProfiler::g_runProfilingSample = false;
				D_LOG_INFO("End Profiling");
			}
		}
	}

	void CPUProfiler::EndSession()
	{
		this->file << "]" << std::endl << "}";
		this->file.close();
	}

	void CPUProfiler::Render(Timestamp delta)
	{
		ImGui::BulletText("FPS: %f", 1.0f / delta.AsSeconds());
		ImGui::BulletText("Frametime (ms): %f", delta.AsMilliSeconds());
	}

}