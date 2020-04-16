#pragma once
#include "IDeviceChild.h"
#include "GraphicsHelpers.h"

namespace LambdaEngine
{
	class IDescriptorSet;
	class IPipelineLayout;

	struct DescriptorHeapDesc
	{
		const char*			pName			= "";
		DescriptorCountDesc DescriptorCount = { };
	};

	class IDescriptorHeap : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IDescriptorHeap);

		/*
		* Returns the count of descriptors left in the heap
		*
		* return - Returns a structure with the count of descriptors and descriptor sets left in the heap
		*/
		virtual DescriptorCountDesc GetHeapStatus() const = 0;
		virtual DescriptorHeapDesc	GetDesc()		const = 0;
	};
}