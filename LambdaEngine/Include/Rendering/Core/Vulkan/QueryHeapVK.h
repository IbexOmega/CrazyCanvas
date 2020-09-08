#pragma once
#include "Rendering/Core/API/QueryHeap.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class QueryHeapVK : public TDeviceChildBase<GraphicsDeviceVK, QueryHeap>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, QueryHeap>;

	public:
		QueryHeapVK(const GraphicsDeviceVK* pDevice);
		~QueryHeapVK();

		bool Init(const QueryHeapDesc* pDesc);

		FORCEINLINE VkQueryPool GetQueryPool() const
		{
			return m_QueryPool;
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// QueryHeap interface
		virtual bool GetResults(uint32 firstQuery, uint32 queryCount, uint64* pData) const override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_QueryPool);
		}

	public:
		VkQueryPool m_QueryPool = VK_NULL_HANDLE;
	};
}