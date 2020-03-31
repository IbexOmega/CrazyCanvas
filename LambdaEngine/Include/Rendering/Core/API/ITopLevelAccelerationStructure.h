#pragma once

#include "IDeviceChild.h"

namespace LambdaEngine
{
	class IBuffer;

	struct TopLevelAccelerationStructureDesc
	{
		const char* pName			= "";
	};

	class ITopLevelAccelerationStructure : public IDeviceChild
	{
	public:
		DECL_INTERFACE(ITopLevelAccelerationStructure);

		virtual void UpdateData(IBuffer* pBuffer) = 0;
	};
}
