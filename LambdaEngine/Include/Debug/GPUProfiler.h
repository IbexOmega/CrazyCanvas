#pragma once
#include "PreCompiled.h"
#include <vulkan/vulkan.h>

namespace LambdaEngine
{
	class CommandList;
	class QueryHeap;

	class GPUProfiler
	{
	public:
		struct Timestamp
		{
			Timestamp() : start(0), end(0), pCommandList(nullptr) {};
			Timestamp(uint64_t start, uint64_t end, CommandList* pCommandList = nullptr) : start(start), end(end), pCommandList(pCommandList) {};

			uint64_t start;
			uint64_t end;
			CommandList* pCommandList;
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
		~GPUProfiler() = default;
		GPUProfiler(GPUProfiler& other) = delete;
		GPUProfiler operator=(GPUProfiler& other) = delete;

		void Init(TimeUnit timeUnit);
		void Cleanup();

		// Create timestamps per command list
		void CreateTimestamps(uint32_t listCount);
		// CreateGraphicsPipelineStats();
		// CreateComputePipelineStats();

		// Timestamps are buffer bound
		void AddTimestamp(CommandList* pCommandList);
		void StartTimestamp(CommandList* pCommandList);
		void EndTimestamp(CommandList* pCommandList);
		void GetTimestamp(CommandList* pCommandList);
		void ResetTimestamp(CommandList* pCommandList);

	public:
		static GPUProfiler* Get();

	private:
		QueryHeap* m_pTimestampHeap = nullptr;
		uint32_t m_TimestampCount = 0;
		TimeUnit m_TimeUnit;
		THashTable<CommandList*, Timestamp> m_Timestamps;
		uint32_t m_NextIndex = 0;
		float m_TimestampPeriod = 0.f;
		uint32_t m_TimestampValidBits = 0;

		THashTable<CommandList*, float> m_Results;

	private:
		static GPUProfiler* s_pInstance;
	};
}