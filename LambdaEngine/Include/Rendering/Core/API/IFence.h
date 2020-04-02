#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
	class IFence : public IDeviceChild
	{
	public:
		DECL_INTERFACE(IFence);

		virtual void Wait(uint64 signalValue, uint64 timeOut) const	= 0;
		virtual void Signal(uint64 signalValue)						= 0;

		virtual uint64 GetValue() const = 0;
	};
}