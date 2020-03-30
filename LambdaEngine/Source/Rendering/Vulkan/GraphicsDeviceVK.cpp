#include "Log/Log.h"

#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
	GraphicsDeviceVK::GraphicsDeviceVK() :
		Instance(VK_NULL_HANDLE),
		Device(VK_NULL_HANDLE),
		PhysicalDevice(VK_NULL_HANDLE)
	{
	}

	GraphicsDeviceVK::~GraphicsDeviceVK()
	{
	}

	bool GraphicsDeviceVK::Init(const GraphicsDeviceDesc& desc)
	{
		//m_ValidationLayers =

		//m_RequiredInstanceExtensions =

		//m_OptionalInstanceExtensions =

		return false;
	}

	void GraphicsDeviceVK::Release()
	{
	}

	IRenderPass* GraphicsDeviceVK::CreateRenderPass()
	{
		return nullptr;
	}

	IFence* GraphicsDeviceVK::CreateFence()
	{
		return nullptr;
	}

	ICommandList* GraphicsDeviceVK::CreateCommandList()
	{
		return nullptr;
	}

	IPipelineState* GraphicsDeviceVK::CreatePipelineState()
	{
		return nullptr;
	}

	IBuffer* GraphicsDeviceVK::CreateBuffer()
	{
		return nullptr;
	}

	ITexture* GraphicsDeviceVK::CreateTexture()
	{
		return nullptr;
	}

	ITextureView* GraphicsDeviceVK::CreateTextureView()
	{
		return nullptr;
	}

	bool GraphicsDeviceVK::InitInstance(const GraphicsDeviceDesc& desc)
	{
		if (desc.Debug)
		{
			if (!ValidationLayersSupported())
			{
				LOG_ERROR("--- Instance: Validation Layers not supported");
				return false;
			}
		}

		return false;
	}

	bool GraphicsDeviceVK::InitDevice()
	{
		return false;
	}

	bool GraphicsDeviceVK::InitPhysicalDevice()
	{
		return false;
	}

	bool GraphicsDeviceVK::InitLogicalDevice()
	{
		return false;
	}

	bool GraphicsDeviceVK::ValidationLayersSupported()
	{
		std::vector<VkLayerProperties> availableValidationLayers;

		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		availableValidationLayers.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableValidationLayers.data());

		for (const char* layerName : m_ValidationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableValidationLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

	int32 GraphicsDeviceVK::RatePhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		return int32();
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsDeviceVK::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		return VK_FALSE;
	}
}
