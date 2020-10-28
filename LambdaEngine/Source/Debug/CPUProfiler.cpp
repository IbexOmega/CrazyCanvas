#include "PreCompiled.h"
#include "Debug/CPUProfiler.h"
#include "Input/API/Input.h"
#include "Application/API/CommonApplication.h"

#include <imgui.h>

// Code modifed from: https://github.com/TheCherno/Hazel

/*
	InstrumentationTimer class
*/

namespace LambdaEngine
{

	InstrumentationTimer::InstrumentationTimer(const std::string& name, bool active) : m_Name(name), m_Active(active)
	{
		Start();
	}

	InstrumentationTimer::~InstrumentationTimer()
	{
		Stop();
	}

	void InstrumentationTimer::Start()
	{
		if (m_Active)
			m_StartTime = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
	}

	void InstrumentationTimer::Stop()
	{
		if (m_Active)
		{
			long long start = m_StartTime;
			long long end = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();

			size_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
			CPUProfiler::Get()->Write({ m_Name, (uint64_t)start, (uint64_t)end, tid });
		}
	}

	/*
		Instrumentation class
	*/

	bool CPUProfiler::g_RunProfilingSample = false;

	CPUProfiler::CPUProfiler() : m_Counter(0)
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
		UNREFERENCED_VARIABLE(name);
		m_Counter = 0;
		m_File.open(filePath);
		m_File << "{\"otherData\": {}, \"displayTimeUnit\": \"ms\", \"traceEvents\": [";
		m_File.flush();
	}

	void CPUProfiler::Write(ProfileData data)
	{
		if (data.PID == 0) {
			data.Start -= m_StartTime;
			data.End -= m_StartTime;
		}

		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_Counter++ > 0) m_File << ",";

		std::string name = data.Name;
		std::replace(name.begin(), name.end(), '"', '\'');

		m_File << "\n{";
		m_File << "\"name\": \"" << name << "\",";
		m_File << "\"cat\": \"function\",";
		m_File << "\"ph\": \"X\",";
		m_File << "\"pid\": " << data.PID << ",";
		m_File << "\"tid\": " << data.TID << ",";
		m_File << "\"ts\": " << data.Start << ",";
		m_File << "\"dur\": " << (data.End - data.Start);
		m_File << "}";

		m_File.flush();
	}

	void CPUProfiler::SetStartTime(uint64_t time)
	{
		m_StartTime = time;
	}

	void CPUProfiler::ToggleSample(EKey key, uint32_t frameCount)
	{
		static uint32_t frameCounter = 0;
		if (Input::GetKeyboardState(EInputLayer::GAME).IsKeyDown(key))
		{
			frameCounter = 0;
			CPUProfiler::g_RunProfilingSample = true;
			D_LOG_INFO("Start Profiling");
		}

		if (CPUProfiler::g_RunProfilingSample)
		{
			frameCounter++;
			if (frameCounter > frameCount)
			{
				CPUProfiler::g_RunProfilingSample = false;
				D_LOG_INFO("End Profiling");
			}
		}
	}

	void CPUProfiler::EndSession()
	{
		m_File << "]" << std::endl << "}";
		m_File.close();
	}

	void CPUProfiler::Tick(Timestamp delta)
	{
		m_TimeSinceUpdate += delta.AsSeconds();
		m_Timestamp = delta;
	}

	void CPUProfiler::Render()
	{
		if (m_TimeSinceUpdate > 1 / m_UpdateFrequency)
		{
			CommonApplication::Get()->GetPlatformApplication()->QueryCPUStatistics(&m_CPUStat);
			m_TimeSinceUpdate = 0.f;
		}
		ImGui::BulletText("FPS: %f", 1.0f / m_Timestamp.AsSeconds());
		ImGui::BulletText("Frametime (ms): %f", m_Timestamp.AsMilliSeconds());
		// Spacing
		ImGui::Dummy(ImVec2(0.0f, 20.0f));

		if (ImGui::CollapsingHeader("CPU Statistics", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Indent(10.0f);
			static const char* items[] = { "B", "KB", "MB", "GB" };
			static int itemSelected = 2;
			static float byteDivider = 1;
			ImGui::Combo("Memory suffix CPU", &itemSelected, items, 4, 4);
			if (itemSelected == 0) { byteDivider = 1.f; }
			if (itemSelected == 1) { byteDivider = 1024.f; }
			if (itemSelected == 2) { byteDivider = 1024.f * 1024.f; }
			if (itemSelected == 3) { byteDivider = 1024.f * 1024.f * 1024.f; }

			ImGui::SliderFloat("Update frequency (per second)", &m_UpdateFrequency, 0.1f, 20.0f, "%.2f");

			float32 percentage = (float32)(m_CPUStat.PhysicalMemoryUsage / (float32)m_CPUStat.PhysicalMemoryAvailable);
			char buf[64];
			sprintf(buf, "%.3f/%.3f (%s)", (float64)m_CPUStat.PhysicalMemoryUsage / byteDivider, (float64)m_CPUStat.PhysicalMemoryAvailable / byteDivider, items[itemSelected]);
			ImGui::Text("Memory usage for process");
			ImGui::ProgressBar(percentage, ImVec2(-1.0f, 0.0f), buf);
			ImGui::Text("Peak memory usage: %.3f (%s)", m_CPUStat.PhysicalPeakMemoryUsage / byteDivider, items[itemSelected]);
			ImGui::Text("CPU Usage for process");
			sprintf(buf, "%.3f%%", m_CPUStat.CPUPercentage);
			ImGui::ProgressBar((float)m_CPUStat.CPUPercentage / 100.f, ImVec2(-1.0f, 0.0f), buf);

			ImGui::Dummy(ImVec2(0.0f, 20.0f));
			ImGui::Unindent(10.0f);
		}
	}

}