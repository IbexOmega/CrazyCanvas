#pragma once
#include "DeviceChild.h"
#include "GraphicsHelpers.h"
#include "DescriptorHeapInfo.h"

namespace LambdaEngine
{
	class DescriptorSet;
	class PipelineLayout;

	struct DescriptorHeapDesc
	{
		String				DebugName			= "";
		uint32				DescriptorSetCount	= 0;
		DescriptorHeapInfo	DescriptorCount		= DescriptorHeapInfo();
	};

	class DescriptorHeap : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(DescriptorHeap);

		/*
		* Returns the API-specific handle to the underlaying QueryHeap-resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;

		/*
		* Returns the count of descriptors left in the heap
		*	return - Returns a structure with the count of descriptors and descriptor sets left in the heap
		*/
		FORCEINLINE const DescriptorHeapInfo& GetHeapStatus() const
		{
			return m_HeapStatus;
		}

		FORCEINLINE const DescriptorHeapDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		DescriptorHeapInfo m_HeapStatus;
		DescriptorHeapDesc m_Desc;
	};
}