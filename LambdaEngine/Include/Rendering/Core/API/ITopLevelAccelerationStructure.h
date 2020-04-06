#pragma once

#include "IDeviceChild.h"

namespace LambdaEngine
{
	class IBuffer;

	struct TopLevelAccelerationStructureDesc
	{
		const char* pName				= "";
		uint32 InitialMaxInstanceCount	= 8;
	};

	class ITopLevelAccelerationStructure : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(ITopLevelAccelerationStructure);

		/*
		* Update the instance data of this TopLevelAS
		*	pInstanceBuffer - A device buffer containing the instances of this TopLevelAS
		*/
		virtual void UpdateInstanceData(IBuffer* pInstanceBuffer) = 0;

		/*
		* Getter for the minimum memory size requirement of a scratch buffer that will be used to build/refit this TopLevelAS
		*
		* return - The minimum size in bytes
		*/
		virtual uint64 GetScratchMemorySizeRequirement() = 0;
	};
}
