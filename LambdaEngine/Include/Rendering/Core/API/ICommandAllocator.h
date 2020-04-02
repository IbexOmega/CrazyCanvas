#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class ICommandAllocator : public IDeviceChild
	{
	public:
		DECL_INTERFACE(ICommandAllocator);

		virtual bool Reset()	= 0;

		virtual uint64		GetHandle() const	= 0;
		virtual ECommandQueueType	GetType()	const	= 0;
	};
}