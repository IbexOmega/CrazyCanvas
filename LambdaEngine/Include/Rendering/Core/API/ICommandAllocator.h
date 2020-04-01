#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
	class ICommandAllocator : public IDeviceChild
	{
	public:
		DECL_INTERFACE(ICommandAllocator);

		virtual void Reset() = 0;
	};
}