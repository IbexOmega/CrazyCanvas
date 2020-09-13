#pragma once
#include "Containers/TArray.h"
#include "Containers/THashTable.h"
#include "Containers/String.h"
#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class CommandList;
	class QueryHeap;

	class GPUProfiler
	{
	public:
		struct Timestamp
		{
			Timestamp() : Start(0), End(0), pCommandList(nullptr), Duration(0.0f), Name("") {};
			Timestamp(uint64_t start, uint64_t end, CommandList* pCommandList = nullptr) : Start(start), End(end), pCommandList(pCommandList), Duration(0.0f), Name("") {};

			uint64_t Start;
			uint64_t End;
			float Duration;
			CommandList* pCommandList;
			String Name;
		};

		struct PlotResult
		{
			String			Name;
			TArray<float>	Results;
		};

		enum class TimeUnit
		{
			NANO = 1,
			MICRO = 1000,
			MILLI = 1000000,
			SECONDS = 1000000000
		};

	public:
		GPUProfiler();
		~GPUProfiler();
		GPUProfiler(GPUProfiler& other) = delete;
		GPUProfiler operator=(GPUProfiler& other) = delete;

		void Init(TimeUnit timeUnit);
		void Render(LambdaEngine::Timestamp delta);
		void Release();

		// Create timestamps per command list
		void CreateTimestamps(uint32_t listCount);
		void CreateGraphicsPipelineStats();
		// CreateComputePipelineStats();

		// Timestamps are buffer bound
		void AddTimestamp(CommandList* pCommandList, const String& name);
		void StartTimestamp(CommandList* pCommandList);
		void EndTimestamp(CommandList* pCommandList);
		void GetTimestamp(CommandList* pCommandList);
		void ResetTimestamp(CommandList* pCommandList);
		void ResetAllTimestamps(CommandList* pCommandList);

		void StartGraphicsPipelineStat(CommandList* pCommandList);
		void EndGraphicsPipelineStat(CommandList* pCommandList);
		void GetGraphicsPipelineStat();
		void ResetGraphicsPipelineStat(CommandList* pCommandList);

	public:
		static GPUProfiler* Get();

	private:
		void SaveResults();
		std::string GetTimeUnitName();

	private:
		// Timestamps
		QueryHeap* m_pTimestampHeap = nullptr;
		TimeUnit m_TimeUnit;
		THashTable<CommandList*, Timestamp> m_Timestamps;
		uint32_t m_TimestampCount		= 0;
		uint32_t m_NextIndex			= 0;
		uint32_t m_TimestampValidBits	= 0;
		float m_TimestampPeriod			= 0.f;
		uint64_t m_StartTimestamp		= 0;

		THashTable<String, Timestamp>	m_Results;
		TArray<PlotResult>				m_PlotResults;
		uint32_t m_PlotResultsStart		= 0;
		size_t m_PlotDataSize;
		THashTable<String, float> m_CurrentMaxDuration;
		float m_TimeSinceUpdate			= 0.0f;
		float m_UpdateFreq;
		bool m_EnableGraph				= true;

		THashTable<CommandList*, bool> m_ShouldGetTimestamps;

		// Pipeline statistics
		QueryHeap* m_pPipelineStatHeap = nullptr;
		TArray<uint64_t> m_GraphicsStats;
	};
}