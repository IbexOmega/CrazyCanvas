#pragma once

#include <vulkan/vulkan.h>

#define GET_INSTANCE_PROC_ADDR(instance, function_name)	if ((function_name = (PFN_##function_name)(vkGetInstanceProcAddr(instance, #function_name))) == nullptr)	{ LOG_WARNING("--- Vulkan: Failed to load Instance-Function '%s'", #function_name); }
#define GET_DEVICE_PROC_ADDR(device, function_name)		if ((function_name = (PFN_##function_name)(vkGetDeviceProcAddr(device, #function_name))) == nullptr)		{ LOG_WARNING("--- Vulkan: Failed to load Device-Function '%s'", #function_name); }
