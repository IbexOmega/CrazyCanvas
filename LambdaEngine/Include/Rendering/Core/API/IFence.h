#pragma once
#include "IDeviceChild.h"

namespace LambdaEngine
{
	class IFence : public IDeviceChild
	{
	public:
		DECL_INTERFACE(IFence);

		virtual void Wait()	const	= 0;
		virtual void Signal()		= 0;

		virtual uint64 GetValue() const = 0;
	};
}