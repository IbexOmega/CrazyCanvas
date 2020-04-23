#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
	FenceVK::FenceVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}
	
	FenceVK::~FenceVK()
	{
	}
	
	bool FenceVK::Init(const FenceDesc* pDesc)
	{
		//LOG_WARNING("[FenceLegacyVK]: 'Init'FUNCTION NOT IMPLEMENTED");
		return true;
	}
	
	void FenceVK::SetName(const char* pName)
	{
		//LOG_WARNING("[FenceLegacyVK]: 'SetName'FUNCTION NOT IMPLEMENTED");
	}
	
	void FenceVK::Wait(uint64 waitValue, uint64 timeOut) const
	{
		//LOG_WARNING("[FenceLegacyVK]: 'Wait'FUNCTION NOT IMPLEMENTED");
	}
	
	void FenceVK::Signal(uint64 signalValue)
	{
		//LOG_WARNING("[FenceLegacyVK]: 'Signal'FUNCTION NOT IMPLEMENTED");
	}

	uint64 FenceVK::GetValue() const
	{
		//LOG_WARNING("[FenceLegacyVK]: 'GetValue' FUNCTION NOT IMPLEMENTED");
		return uint64(0);
	}
}
