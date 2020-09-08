#include "PreCompiled.h"
#include "Debug/GPUProfiler.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/QueryHeap.h"
#include "Rendering/RenderSystem.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandQueue.h"
#include <glm/glm.hpp>

namespace LambdaEngine
{
	GPUProfiler* GPUProfiler::s_pInstance = nullptr;

	GPUProfiler::GPUProfiler() : m_TimeUnit(TimeUnit::MILLI)
	{
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
		bool res = m_pTimestampHeap->GetResults(m_Timestamps[pCommandList].start, timestampCount, results.GetData());

		if (res)
		{
			uint64_t start = glm::bitfieldExtract<uint64_t>(results[0], 0, m_TimestampValidBits);
			uint64_t end = glm::bitfieldExtract<uint64_t>(results[1], 0, m_TimestampValidBits);

			m_Results[pCommandList] = ((end - start) * m_TimestampPeriod) / (uint64_t)m_TimeUnit;
			D_LOG_INFO("Time: %f", m_Results[pCommandList]);
		}
	}

	void GPUProfiler::ResetTimestamp(CommandList* pCommandList)
	{
		uint32_t firstQuery = m_Timestamps[pCommandList].start;
		pCommandList->ResetQuery(m_pTimestampHeap, firstQuery, 2);
	}

	GPUProfiler* GPUProfiler::Get()
	{
		if (s_pInstance == nullptr)
			s_pInstance = DBG_NEW GPUProfiler();
		return s_pInstance;
	}
}