#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FenceLegacyVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
	FenceLegacyVK::FenceLegacyVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}
	
	FenceLegacyVK::~FenceLegacyVK()
	{
	}
	
	bool FenceLegacyVK::Init(const FenceDesc* pDesc)
	{
		//LOG_WARNING("[FenceLegacyVK]: 'Init'FUNCTION NOT IMPLEMENTED");
		return true;
	}
	
	void FenceLegacyVK::SetName(const char* pName)
	{
		//LOG_WARNING("[FenceLegacyVK]: 'SetName'FUNCTION NOT IMPLEMENTED");
	}
	
	void FenceLegacyVK::Wait(uint64 waitValue, uint64 timeOut) const
	{
		//LOG_WARNING("[FenceLegacyVK]: 'Wait'FUNCTION NOT IMPLEMENTED");
	}
	
	void FenceLegacyVK::Signal(uint64 signalValue)
	{
		//LOG_WARNING("[FenceLegacyVK]: 'Signal'FUNCTION NOT IMPLEMENTED");
	}

	uint64 FenceLegacyVK::GetValue() const
	{
		//LOG_WARNING("[FenceLegacyVK]: 'GetValue' FUNCTION NOT IMPLEMENTED");
		return uint64(0);
	}
}
