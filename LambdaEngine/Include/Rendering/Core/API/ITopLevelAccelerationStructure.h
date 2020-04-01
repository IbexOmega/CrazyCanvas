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
		DECL_INTERFACE(ITopLevelAccelerationStructure);

		virtual void UpdateInstanceData(IBuffer* pInstanceBuffer) = 0;

		virtual uint64 GetScratchMemorySizeRequirement() = 0;
	};
}
