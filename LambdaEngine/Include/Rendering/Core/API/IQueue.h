#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
	class IFence;
	class ICommandList;

	class IQueue : public IDeviceChild
	{
	public:
		DECL_INTERFACE(IQueue);

		virtual bool ExecuteCommandList(const ICommandList* const* ppCommandList, uint32 numCommandLists, const IFence* pFence) = 0;
		virtual void Wait() = 0;

		virtual uint64 GetHandle() const = 0;
	};
}