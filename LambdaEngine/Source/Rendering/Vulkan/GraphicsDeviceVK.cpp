#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Log/Log.h"

#define GET_INSTANCE_PROC_ADDR(instance, function_name) if ((function_name = reinterpret_cast<PFN_##function_name>(vkGetInstanceProcAddr(instance, #function_name))) == nullptr) { LOG_ERROR("--- Vulkan: Failed to load InstanceFunction '%s'", #function_name); }

namespace LambdaEngine
{
	constexpr ValidationLayer REQUIRED_VALIDATION_LAYERS[]
	{
		ValidationLayer("VK_LAYER_KHRONOS_validation")
	};

	constexpr ValidationLayer OPTIONAL_VALIDATION_LAYERS[]
	{
		ValidationLayer("VK_LAYER_OPTIONAL_TEST")
	};

	constexpr Extension REQUIRED_INSTANCE_EXTENSIONS[]
	{
		Extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
	};

	constexpr Extension OPTIONAL_INSTANCE_EXTENSIONS[]
	{
		Extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
	};

	GraphicsDeviceVK::GraphicsDeviceVK() :
		Instance(VK_NULL_HANDLE),
		Device(VK_NULL_HANDLE),
		PhysicalDevice(VK_NULL_HANDLE),
		vkSetDebugUtilsObjectNameEXT(nullptr),
		vkDestroyDebugUtilsMessengerEXT(nullptr),
		vkCreateDebugUtilsMessengerEXT(nullptr),
		m_DebugMessenger(VK_NULL_HANDLE)
	{
	}

	GraphicsDeviceVK::~GraphicsDeviceVK()
	{
		if (m_DebugMessenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(Instance, m_DebugMessenger, nullptr);
			m_DebugMessenger = VK_NULL_HANDLE;
		}

		if (Instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(Instance, nullptr);
			Instance = VK_NULL_HANDLE;
		}
	}

	bool GraphicsDeviceVK::Init(const GraphicsDeviceDesc& desc)
	{
		if (!InitInstance(desc))

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
			if (!SetEnabledValidationLayers())
			{
				LOG_ERROR("--- GraphicsDeviceVK: Validation Layers not supported");
				return false;
			}
		}

		if (!SetEnabledInstanceExtensions())
		{
			LOG_ERROR("--- GraphicsDeviceVK: Required Instance Extensions not supported");
			return false;
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = "Vulkan Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = (uint32_t)m_EnabledInstanceExtensions.size();
		createInfo.ppEnabledExtensionNames = m_EnabledInstanceExtensions.data();

		if (desc.Debug)
		{
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
			PopulateDebugMessengerCreateInfo(debugCreateInfo);

			createInfo.enabledLayerCount = (uint32_t)m_EnabledValidationLayers.size();
			createInfo.ppEnabledLayerNames = m_EnabledValidationLayers.data();
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &Instance) != VK_SUCCESS)
		{
			LOG_ERROR("--- GraphicsDeviceVK: Failed to create Vulkan Instance!");
			return false;
		}

		RegisterExtensionFunctions();

		if (desc.Debug)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			PopulateDebugMessengerCreateInfo(createInfo);

			if (vkCreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &m_DebugMessenger))
			{
				LOG_ERROR("--- GraphicsDeviceVK: Failed to set up Debug Messenger!");
				return false;
			}
		}

		return true;
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

	bool GraphicsDeviceVK::SetEnabledValidationLayers()
	{
		std::vector<VkLayerProperties> availableValidationLayers;

		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		availableValidationLayers.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableValidationLayers.data());

		std::vector<ValidationLayer> requiredValidationLayers(REQUIRED_VALIDATION_LAYERS, REQUIRED_VALIDATION_LAYERS + sizeof(REQUIRED_VALIDATION_LAYERS) / sizeof(ValidationLayer));
		std::vector<ValidationLayer> optionalValidationLayers(OPTIONAL_VALIDATION_LAYERS, OPTIONAL_VALIDATION_LAYERS + sizeof(OPTIONAL_VALIDATION_LAYERS) / sizeof(ValidationLayer));

		for (const VkLayerProperties& availableValidationLayerProperties : availableValidationLayers)
		{
			uint32 availableValidationLayerHash = HashString<const char*>(availableValidationLayerProperties.layerName);

			for (auto requiredValidationLayer = requiredValidationLayers.begin(); requiredValidationLayer != requiredValidationLayers.end(); requiredValidationLayer++)
			{
				if (availableValidationLayerHash == requiredValidationLayer->Hash)
				{
					m_EnabledValidationLayers.push_back(requiredValidationLayer->Name);
					requiredValidationLayers.erase(requiredValidationLayer);
					break;
				}
			}

			for (auto optionalValidationLayer = optionalValidationLayers.begin(); optionalValidationLayer != optionalValidationLayers.end(); optionalValidationLayer++)
			{
				if (availableValidationLayerHash == optionalValidationLayer->Hash)
				{
					m_EnabledValidationLayers.push_back(optionalValidationLayer->Name);
					optionalValidationLayers.erase(optionalValidationLayer);
					break;
				}
			}
		}

		if (requiredValidationLayers.size() > 0)
		{
			for (const ValidationLayer& requiredValidationLayer : requiredValidationLayers)
			{
				LOG_ERROR("--- GraphicsDeviceVK: Required Validation Layer %s not supported", requiredValidationLayer.Name);
			}

			return false;
		}

		if (optionalValidationLayers.size() > 0)
		{
			for (const ValidationLayer& optionalValidationLayer : optionalValidationLayers)
			{
				LOG_WARNING("--- GraphicsDeviceVK: Optional Validation Layer %s not supported", optionalValidationLayer.Name);
			}
		}

		return true;
	}

	bool GraphicsDeviceVK::SetEnabledInstanceExtensions()
	{
		std::vector<VkExtensionProperties> availableExtensions;

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		availableExtensions.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

		std::vector<Extension> requiredInstanceExtensions(REQUIRED_INSTANCE_EXTENSIONS, REQUIRED_INSTANCE_EXTENSIONS + sizeof(REQUIRED_INSTANCE_EXTENSIONS) / sizeof(Extension));
		std::vector<Extension> optionalInstanceExtensions(OPTIONAL_INSTANCE_EXTENSIONS, OPTIONAL_INSTANCE_EXTENSIONS + sizeof(OPTIONAL_INSTANCE_EXTENSIONS) / sizeof(Extension));

		for (const VkExtensionProperties& extension : availableExtensions)
		{
			uint32 availableExtensionHash = HashString<const char*>(extension.extensionName);

			for (auto requiredExtension = requiredInstanceExtensions.begin(); requiredExtension != requiredInstanceExtensions.end(); requiredExtension++)
			{
				if (requiredExtension->Hash == availableExtensionHash)
				{
					m_EnabledInstanceExtensions.push_back(requiredExtension->Name);
					requiredInstanceExtensions.erase(requiredExtension);
					break;
				}
			}

			for (auto optionalExtension = optionalInstanceExtensions.begin(); optionalExtension != optionalInstanceExtensions.end(); optionalExtension++)
			{
				if (optionalExtension->Hash == availableExtensionHash)
				{
					m_EnabledInstanceExtensions.push_back(optionalExtension->Name);
					optionalInstanceExtensions.erase(optionalExtension);
					break;
				}
			}
		}

		if (requiredInstanceExtensions.size() > 0)
		{
			for (const Extension& requiredExtension : requiredInstanceExtensions)
			{
				LOG_ERROR("--- GraphicsDeviceVK: Required Instance Extension %s not supported", requiredExtension.Name);
			}

			return false;
		}

		if (optionalInstanceExtensions.size() > 0)
		{
			for (const Extension& optionalExtension : optionalInstanceExtensions)
			{
				LOG_WARNING("--- GraphicsDeviceVK: Optional Instance Extension %s not supported", optionalExtension.Name);
			}
		}

		return true;
	}

	void GraphicsDeviceVK::RegisterExtensionFunctions()
	{
		GET_INSTANCE_PROC_ADDR(Instance, vkCreateDebugUtilsMessengerEXT);
		GET_INSTANCE_PROC_ADDR(Instance, vkDestroyDebugUtilsMessengerEXT);
		GET_INSTANCE_PROC_ADDR(Instance, vkSetDebugUtilsObjectNameEXT);
	}

	bool GraphicsDeviceVK::IsInstanceExtensionEnabled(const char* extensionName)
	{
		uint32 extensionNameHash = HashString<const char*>(extensionName);

		for (const char* enabledExtension : m_EnabledInstanceExtensions)
		{
			uint32 enabledExtensionNameHash = HashString<const char*>(extensionName);

			if (extensionNameHash == enabledExtensionNameHash)
			{
				return true;
			}
		}

		return false;
	}

	void GraphicsDeviceVK::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
		createInfo.pUserData = nullptr;
	}

	int32 GraphicsDeviceVK::RatePhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		return int32();
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsDeviceVK::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			LOG_MESSAGE("--- Validation Layer: %s", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			LOG_MESSAGE("--- Validation Layer: %s", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LOG_WARNING("--- Validation Layer: %s", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LOG_ERROR("--- Validation Layer: %s", pCallbackData->pMessage);
		}

		return VK_FALSE;
	}
}