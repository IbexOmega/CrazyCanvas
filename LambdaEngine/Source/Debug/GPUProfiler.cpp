#include "PreCompiled.h"
#include "Debug/GPUProfiler.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/QueryHeap.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandQueue.h"

#include <glm/glm.hpp>
#include <imgui.h>

#include <fstream>
#include <sstream>

namespace LambdaEngine
{
	GPUProfiler::GPUProfiler() : m_TimeUnit(TimeUnit::MILLI), m_PlotDataSize(100), m_UpdateFreq(1.0f)
	{
		m_PlotResults.Resize(m_PlotDataSize);
		for (size_t i = 0; i < m_PlotDataSize; i++)
			m_PlotResults[i] = 0.0f;
	}

	GPUProfiler::~GPUProfiler()
	{
		//SaveResults();
	}

	void GPUProfiler::Init(TimeUnit timeUnit)
	{
		GraphicsDeviceFeatureDesc desc = {};
		RenderSystem::GetDevice()->QueryDeviceFeatures(&desc);
		m_TimestampPeriod = desc.TimestampPeriod;

		CommandQueueProperties prop = {};
		RenderSystem::GetGraphicsQueue()->QueryQueueProperties(&prop);
		m_TimestampValidBits = prop.TimestampValidBits;

		m_TimeUnit = timeUnit;
	}

	void GPUProfiler::Render(LambdaEngine::Timestamp delta)
	{
		m_TimeSinceUpdate += delta.AsMilliSeconds();

		if (m_TimestampCount != 0 && ImGui::CollapsingHeader("Timestamps") && m_TimeSinceUpdate > 1 / m_UpdateFreq)
		{
			m_TimeSinceUpdate = 0.0f;
			float average = 0.0f;

			TArray<float> plotData(m_PlotDataSize);
			uint32_t index = m_PlotResultsStart;
			for (size_t i = 0; i < m_PlotDataSize; i++)
			{
				average += m_PlotResults[i];
				plotData[i] = m_PlotResults[index];
				index = (index + 1) % m_PlotDataSize;
			}
			average /= m_PlotDataSize;

			std::ostringstream overlay;
			overlay.precision(2);
			overlay << "Average: " << std::fixed << average << GetTimeUnitName();

			std::string s = "GPU Timestamp Timings";
			ImGui::Text(s.c_str());
			ImGui::PlotLines("", plotData.GetData(), (int)m_PlotDataSize, 0, overlay.str().c_str(), 0.f, m_CurrentMaxDuration, { 0, 80 });
		}

		if (m_pPipelineStatHeap != nullptr && ImGui::CollapsingHeader("Pipeline Stats"))
		{
			const TArray<std::string> statNames = {
				"Input assembly vertex count        ",
				"Input assembly primitives count    ",
				"Vertex shader invocations          ",
				"Clipping stage primitives processed",
				"Clipping stage primtives output    ",
				"Fragment shader invocations        "
			};

			for (size_t i = 0; i < m_GraphicsStats.GetSize(); i++) {
				std::string caption = statNames[i] + ": %d";
				ImGui::BulletText(caption.c_str(), m_GraphicsStats[i]);
			}
		}
	}

	void GPUProfiler::Cleanup()
	{
	}

	void GPUProfiler::CreateTimestamps(uint32_t listCount)
	{
		// Need two timestamps per list
		m_TimestampCount = listCount * 2;

		QueryHeapDesc createInfo = {};
		createInfo.DebugName = "VulkanProfiler Timestamp Heap";
		createInfo.PipelineStatisticsFlags = 0;
		createInfo.QueryCount = m_TimestampCount;
		createInfo.Type = EQueryType::QUERY_TYPE_TIMESTAMP;

		m_pTimestampHeap = RenderSystem::GetDevice()->CreateQueryHeap(&createInfo);
	}

	void GPUProfiler::CreateGraphicsPipelineStats()
	{
		QueryHeapDesc createInfo = {};
		createInfo.DebugName = "VulkanProfiler Graphics Pipeline Statistics Heap";
		createInfo.PipelineStatisticsFlags =
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_VERTICES |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_INPUT_ASSEMBLY_PRIMITIVES |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_VERTEX_SHADER_INVOCATIONS |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_INVOCATIONS |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_CLIPPING_PRIMITIVES |
			FQueryPipelineStatisticsFlag::QUERY_PIPELINE_STATISTICS_FLAG_FRAGMENT_SHADER_INVOCATIONS;
		createInfo.QueryCount = 6;
		createInfo.Type = EQueryType::QUERY_TYPE_PIPELINE_STATISTICS;

		m_pPipelineStatHeap = RenderSystem::GetDevice()->CreateQueryHeap(&createInfo);

		m_GraphicsStats.Resize(createInfo.QueryCount);
	}

	void GPUProfiler::AddTimestamp(CommandList* pCommandList)
	{
		if (m_Timestamps.find(pCommandList) == m_Timestamps.end())
		{
			m_Timestamps[pCommandList].pCommandList = pCommandList;
			m_Timestamps[pCommandList].start = m_NextIndex++;
			m_Timestamps[pCommandList].end = m_NextIndex++;
		}
	}

	void GPUProfiler::StartTimestamp(CommandList* pCommandList)
	{
		// Assume VK_PIPELINE_STAGE_TOP_OF_PIPE or VK_PIPELINE_STAGE_BOTTOM_OF_PIPE;
		pCommandList->Timestamp(m_pTimestampHeap, m_Timestamps[pCommandList].start, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM);
	}

	void GPUProfiler::EndTimestamp(CommandList* pCommandList)
	{
		pCommandList->Timestamp(m_pTimestampHeap, m_Timestamps[pCommandList].end, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM);
	}

	void GPUProfiler::GetTimestamp(CommandList* pCommandList)
	{
		size_t timestampCount = 2;
		TArray<uint64_t> results(timestampCount);
		bool res = m_pTimestampHeap->GetResults(m_Timestamps[pCommandList].start, timestampCount, timestampCount * sizeof(uint64), results.GetData());

		if (res)
		{
			uint64_t start = glm::bitfieldExtract<uint64_t>(results[0], 0, m_TimestampValidBits);
			uint64_t end = glm::bitfieldExtract<uint64_t>(results[1], 0, m_TimestampValidBits);

			if (m_StartTimestamp == 0)
				m_StartTimestamp = start;

			m_Results[pCommandList].start = start;
			m_Results[pCommandList].end = end;
			float duration = ((end - start) * m_TimestampPeriod) / (uint64_t)m_TimeUnit;
			m_Results[pCommandList].duration = duration;

			if (duration > m_CurrentMaxDuration)
				m_CurrentMaxDuration = duration;

			m_PlotResults[m_PlotResultsStart] = duration;
			m_PlotResultsStart = (m_PlotResultsStart + 1) % m_PlotDataSize;
		}
	}

	void GPUProfiler::ResetTimestamp(CommandList* pCommandList)
	{
		uint32_t firstQuery = m_Timestamps[pCommandList].start;
		pCommandList->ResetQuery(m_pTimestampHeap, firstQuery, 2);
	}

	void GPUProfiler::StartGraphicsPipelineStat(CommandList* pCommandList)
	{
		pCommandList->BeginQuery(m_pPipelineStatHeap, 0);
	}

	void GPUProfiler::EndGraphicsPipelineStat(CommandList* pCommandList)
	{
		pCommandList->EndQuery(m_pPipelineStatHeap, 0);
	}

	void GPUProfiler::GetGraphicsPipelineStat()
	{
		m_pPipelineStatHeap->GetResults(0, 1, m_GraphicsStats.GetSize() * sizeof(uint64), m_GraphicsStats.GetData());
	}

	void GPUProfiler::ResetGraphicsPipelineStat(CommandList* pCommandList)
	{
		pCommandList->ResetQuery(m_pPipelineStatHeap, 0, 6);
	}

	GPUProfiler* GPUProfiler::Get()
	{
		static GPUProfiler instance;
		return &instance;
	}

	void GPUProfiler::SaveResults()
	{
		const char* filePath = "GPUResults.txt";

		std::ofstream file;
		file.open(filePath);
		file << "{\"otherData\": {}, \"displayTimeUnit\": \"ms\", \"traceEvents\": [";
		file.flush();

		uint32_t j = 0;
		for (auto& res : m_Results)
		{
			//for (uint32_t i = 0; i < res.second.size(); i++, j++)
			//{
				if (j > 0) file << ",";

				std::string name = "Render Graph Graphics Command List";
				std::replace(name.begin(), name.end(), '"', '\'');

				file << "{";
				file << "\"name\": \"" << name << " " << j << "\",";
				file << "\"cat\": \"function\",";
				file << "\"ph\": \"X\",";
				file << "\"pid\": " << 1 << ",";
				file << "\"tid\": " << 0 << ",";
				file << "\"ts\": " << (((res.second.start - m_StartTimestamp) * m_TimestampPeriod) / (uint64_t)m_TimeUnit) << ",";
				file << "\"dur\": " << res.second.duration;
				file << "}";
				j++;
			//}
		}

		file << "]" << std::endl << "}";
		file.flush();
		file.close();
	}

	std::string GPUProfiler::GetTimeUnitName()
	{
		std::string buf;
		switch (m_TimeUnit) {
		case TimeUnit::MICRO:
			buf = "us";
			break;
		case TimeUnit::MILLI:
			buf = "ms";
			break;
		case TimeUnit::NANO:
			buf = "ns";
			break;
		case TimeUnit::SECONDS:
			buf = "s";
			break;
		}
		return buf;
	}
}