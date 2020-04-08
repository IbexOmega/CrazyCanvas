#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

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
		* Creates a descriptorset from this heap. This modifies the heapstatus
		*
		* pName				- Name of descriptorset. Equal to a call to SetName.
		* pPipelineLayout	- The layout that should be used together with the descriptorset. The amount
		*					of descriptors specified in the pipelinelayout will be allocated to the descriptorset.
		*
		* return			- If creation is successful then a valid pointer will be returned, otherwise nullptr
		*/
		virtual IDescriptorSet* CreateDescriptorSet(const char* pName, const IPipelineLayout* pPipelineLayout) = 0;

		/*
		* Returns the count of descriptors left in the heap
		*
		* return - Returns a structure with the count of descriptors and descriptor sets left in the heap
		*/
		virtual DescriptorCountDesc GetHeapStatus() const = 0;
		virtual DescriptorHeapDesc	GetDesc()		const = 0;
	};
}