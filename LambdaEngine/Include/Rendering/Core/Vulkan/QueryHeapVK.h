#pragma once
#include "Rendering/Core/API/IQueryHeap.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class QueryHeapVK : public TDeviceChildBase<GraphicsDeviceVK, IQueryHeap>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IQueryHeap>;

	public:
		QueryHeapVK(const GraphicsDeviceVK* pDevice);
		~QueryHeapVK();

		bool Init(const QueryHeapDesc* pDesc);

		FORCEINLINE VkQueryPool GetQueryPool() const
		{
			return m_QueryPool;
		}

		// IDeviceChild interface
		virtual void SetName(const char* pName) override final;

		// IQueryHeap interface
		virtual bool GetResults(uint32 firstQuery, uint32 queryCount, uint64* pData) const override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_QueryPool;
		}

		FORCEINLINE virtual QueryHeapDesc GetDesc() const override final
		{
			return m_Desc;
		}

	public:
		VkQueryPool		m_QueryPool = VK_NULL_HANDLE;
		QueryHeapDesc	m_Desc;
	};
}