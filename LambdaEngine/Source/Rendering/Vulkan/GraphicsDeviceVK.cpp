#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Log/Log.h"

#include <set>
#include <map>

#define GET_INSTANCE_PROC_ADDR(instance, function_name) if ((function_name = reinterpret_cast<PFN_##function_name>(vkGetInstanceProcAddr(instance, #function_name))) == nullptr) { LOG_ERROR("--- Vulkan: Failed to load InstanceFunction '%s'", #function_name); }

namespace LambdaEngine
{
	constexpr ValidationLayer REQUIRED_VALIDATION_LAYERS[]
	{
		ValidationLayer("VK_LAYER_KHRONOS_validation")
	};

	constexpr ValidationLayer OPTIONAL_VALIDATION_LAYERS[]
	{
		ValidationLayer("VK_VALIDATION_LAYER_OPTIONAL_TEST")
	};

	constexpr Extension REQUIRED_INSTANCE_EXTENSIONS[]
	{
		Extension(VK_KHR_SURFACE_EXTENSION_NAME),
		Extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
	};

	constexpr Extension OPTIONAL_INSTANCE_EXTENSIONS[]
	{
		Extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
	};

	constexpr Extension REQUIRED_DEVICE_EXTENSIONS[]
	{
		Extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
	};

	constexpr Extension OPTIONAL_DEVICE_EXTENSIONS[]
	{
		Extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME)
	};

	GraphicsDeviceVK::GraphicsDeviceVK() :
		Instance(VK_NULL_HANDLE),
		Device(VK_NULL_HANDLE),
		PhysicalDevice(VK_NULL_HANDLE),
		vkSetDebugUtilsObjectNameEXT(nullptr),
		vkDestroyDebugUtilsMessengerEXT(nullptr),
		vkCreateDebugUtilsMessengerEXT(nullptr),
		m_DebugMessenger(VK_NULL_HANDLE),
		m_GraphicsQueue(VK_NULL_HANDLE),
		m_ComputeQueue(VK_NULL_HANDLE),
		m_TransferQueue(VK_NULL_HANDLE),
		m_PresentQueue(VK_NULL_HANDLE),
		m_DeviceLimits({})
	{
	}

	GraphicsDeviceVK::~GraphicsDeviceVK()
	{
		if (Device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(Device, nullptr);
			Device = VK_NULL_HANDLE;
		}

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
		{
			LOG_ERROR("--- GraphicsDeviceVK: Vulkan Instance could not be initialized!");
			return false;
		}
		else
		{
			LOG_MESSAGE("--- GraphicsDeviceVK: Vulkan Instance initialized!");
		}

		if (!InitDevice(desc))
		{
			LOG_ERROR("--- GraphicsDeviceVK: Vulkan Device could not be initialized!");
			return false;
		}
		else
		{
			LOG_MESSAGE("--- GraphicsDeviceVK: Vulkan Device initialized!");
		}
	}

	void GraphicsDeviceVK::Release()
	{
		delete this;
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

	void GraphicsDeviceVK::ExecuteGraphics(CommandBufferVK* pCommandBuffer, const VkSemaphore* pWaitSemaphore, const VkPipelineStageFlags* pWaitStages, uint32_t waitSemaphoreCount, const VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount)
	{
		LOG_ERROR("Call to unimplemented function GraphicsDeviceVK::ExecuteGraphics");
	}

	void GraphicsDeviceVK::ExecuteCompute(CommandBufferVK* pCommandBuffer, const VkSemaphore* pWaitSemaphore, const VkPipelineStageFlags* pWaitStages, uint32_t waitSemaphoreCount, const VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount)
	{
		LOG_ERROR("Call to unimplemented function GraphicsDeviceVK::ExecuteCompute");
	}

	void GraphicsDeviceVK::ExecuteTransfer(CommandBufferVK* pCommandBuffer, const VkSemaphore* pWaitSemaphore, const VkPipelineStageFlags* pWaitStages, uint32_t waitSemaphoreCount, const VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount)
	{
		LOG_ERROR("Call to unimplemented function GraphicsDeviceVK::ExecuteTransfer");
	}

	void GraphicsDeviceVK::SetVulkanObjectName(const char* pName, uint64_t objectHandle, VkObjectType type)
	{
		if (pName)
		{
			if (vkSetDebugUtilsObjectNameEXT)
			{
				VkDebugUtilsObjectNameInfoEXT info = {};
				info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
				info.pNext = nullptr;
				info.objectType = type;
				info.objectHandle = objectHandle;
				info.pObjectName = pName;
				vkSetDebugUtilsObjectNameEXT(Device, &info);
			}
		}
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

	bool GraphicsDeviceVK::InitDevice(const GraphicsDeviceDesc& desc)
	{
		if (!InitPhysicalDevice())
		{
			LOG_ERROR("--- GraphicsDeviceVK: Could not initialize Physical Device!");
			return false;
		}

		if (!InitLogicalDevice(desc))
		{
			LOG_ERROR("--- GraphicsDeviceVK: Could not initialize Logical Device!");
			return false;
		}

		return true;
	}

	bool GraphicsDeviceVK::InitPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			LOG_ERROR("--- GraphicsDeviceVK: Presentation is not supported by the selected physicaldevice");
			return false;
		}

		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(Instance, &deviceCount, physicalDevices.data());

		std::multimap<int32, VkPhysicalDevice> physicalDeviceCandidates;

		for (VkPhysicalDevice physicalDevice : physicalDevices)
		{
			int32 score = RatePhysicalDevice(physicalDevice);
			physicalDeviceCandidates.insert(std::make_pair(score, physicalDevice));
		}

		// Check if the best candidate is suitable at all
		if (physicalDeviceCandidates.rbegin()->first <= 0)
		{
			LOG_ERROR("--- GraphicsDeviceVK: Failed to find a suitable GPU!");
			return false;
		}

		PhysicalDevice = physicalDeviceCandidates.rbegin()->second;
		SetEnabledDeviceExtensions();
		m_DeviceQueueFamilyIndices = FindQueueFamilies(PhysicalDevice);

		// Save device's limits
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(PhysicalDevice, &deviceProperties);
		m_DeviceLimits = deviceProperties.limits;

		return true;
	}

	bool GraphicsDeviceVK::InitLogicalDevice(const GraphicsDeviceDesc& desc)
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32> uniqueQueueFamilies =
		{
			m_DeviceQueueFamilyIndices.GraphicsFamily.value(),
			m_DeviceQueueFamilyIndices.ComputeFamily.value(),
			m_DeviceQueueFamilyIndices.TransferFamily.value(),
			m_DeviceQueueFamilyIndices.PresentFamily.value()
		};

		float queuePriority = 1.0f;
		for (uint32 queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.fillModeNonSolid = true;
		deviceFeatures.vertexPipelineStoresAndAtomics = true;
		deviceFeatures.fragmentStoresAndAtomics = true;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = (uint32)queueCreateInfos.size();
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = (uint32)m_EnabledDeviceExtensions.size();
		createInfo.ppEnabledExtensionNames = m_EnabledDeviceExtensions.data();

		if (desc.Debug)
		{
			createInfo.enabledLayerCount = (uint32)m_EnabledValidationLayers.size();
			createInfo.ppEnabledLayerNames = m_EnabledValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device) != VK_SUCCESS)
		{
			LOG_ERROR("--- GraphicsDeviceVK: Failed to create logical device!");
			return false;
		}

		//Retrive queues
		vkGetDeviceQueue(Device, m_DeviceQueueFamilyIndices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(Device, m_DeviceQueueFamilyIndices.PresentFamily.value(), 0, &m_PresentQueue);
		SetVulkanObjectName("GraphicsQueue", (uint64_t)m_GraphicsQueue, VK_OBJECT_TYPE_QUEUE);

		vkGetDeviceQueue(Device, m_DeviceQueueFamilyIndices.ComputeFamily.value(), 0, &m_ComputeQueue);
		SetVulkanObjectName("ComputeQueue", (uint64_t)m_ComputeQueue, VK_OBJECT_TYPE_QUEUE);

		vkGetDeviceQueue(Device, m_DeviceQueueFamilyIndices.TransferFamily.value(), 0, &m_TransferQueue);
		SetVulkanObjectName("TransferQueue", (uint64_t)m_TransferQueue, VK_OBJECT_TYPE_QUEUE);

		return true;
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
		std::vector<VkExtensionProperties> availableInstanceExtensions;

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		availableInstanceExtensions.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data());

		std::vector<Extension> requiredInstanceExtensions(REQUIRED_INSTANCE_EXTENSIONS, REQUIRED_INSTANCE_EXTENSIONS + sizeof(REQUIRED_INSTANCE_EXTENSIONS) / sizeof(Extension));
		std::vector<Extension> optionalInstanceExtensions(OPTIONAL_INSTANCE_EXTENSIONS, OPTIONAL_INSTANCE_EXTENSIONS + sizeof(OPTIONAL_INSTANCE_EXTENSIONS) / sizeof(Extension));

		for (const VkExtensionProperties& extension : availableInstanceExtensions)
		{
			uint32 availableInstanceExtensionHash = HashString<const char*>(extension.extensionName);

			for (auto requiredInstanceExtension = requiredInstanceExtensions.begin(); requiredInstanceExtension != requiredInstanceExtensions.end(); requiredInstanceExtension++)
			{
				if (requiredInstanceExtension->Hash == availableInstanceExtensionHash)
				{
					m_EnabledInstanceExtensions.push_back(requiredInstanceExtension->Name);
					requiredInstanceExtensions.erase(requiredInstanceExtension);
					break;
				}
			}

			for (auto optionalInstanceExtension = optionalInstanceExtensions.begin(); optionalInstanceExtension != optionalInstanceExtensions.end(); optionalInstanceExtension++)
			{
				if (optionalInstanceExtension->Hash == availableInstanceExtensionHash)
				{
					m_EnabledInstanceExtensions.push_back(optionalInstanceExtension->Name);
					optionalInstanceExtensions.erase(optionalInstanceExtension);
					break;
				}
			}
		}

		if (requiredInstanceExtensions.size() > 0)
		{
			for (const Extension& requiredInstanceExtension : requiredInstanceExtensions)
			{
				LOG_ERROR("--- GraphicsDeviceVK: Required Instance Extension %s not supported", requiredInstanceExtension.Name);
			}

			return false;
		}

		if (optionalInstanceExtensions.size() > 0)
		{
			for (const Extension& optionalInstanceExtension : optionalInstanceExtensions)
			{
				LOG_WARNING("--- GraphicsDeviceVK: Optional Instance Extension %s not supported", optionalInstanceExtension.Name);
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
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		bool requiredExtensionsSupported = false;
		uint32_t numOfOptionalExtensionsSupported = 0;
		CheckDeviceExtensionsSupport(physicalDevice, requiredExtensionsSupported, numOfOptionalExtensionsSupported);

		if (!requiredExtensionsSupported)
			return 0;

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

		if (!indices.IsComplete())
			return 0;

		int score = 1 + numOfOptionalExtensionsSupported;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;

		return score;
	}

	void GraphicsDeviceVK::CheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice, bool& requiredExtensionsSupported, uint32_t& numOfOptionalExtensionsSupported)
	{
		std::vector<VkExtensionProperties> availableDeviceExtensions;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		availableDeviceExtensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data());

		std::vector<Extension> requiredDeviceExtensions(REQUIRED_DEVICE_EXTENSIONS, REQUIRED_DEVICE_EXTENSIONS + sizeof(REQUIRED_DEVICE_EXTENSIONS) / sizeof(Extension));
		std::vector<Extension> optionalDeviceExtensions(OPTIONAL_DEVICE_EXTENSIONS, OPTIONAL_DEVICE_EXTENSIONS + sizeof(OPTIONAL_DEVICE_EXTENSIONS) / sizeof(Extension));

		for (const VkExtensionProperties& extension : availableDeviceExtensions)
		{
			uint32 availableDeviceExtensionHash = HashString<const char*>(extension.extensionName);

			for (auto requiredDeviceExtension = requiredDeviceExtensions.begin(); requiredDeviceExtension != requiredDeviceExtensions.end(); requiredDeviceExtension++)
			{
				if (requiredDeviceExtension->Hash == availableDeviceExtensionHash)
				{
					requiredDeviceExtensions.erase(requiredDeviceExtension);
					break;
				}
			}

			for (auto optionalDeviceExtension = optionalDeviceExtensions.begin(); optionalDeviceExtension != optionalDeviceExtensions.end(); optionalDeviceExtension++)
			{
				if (optionalDeviceExtension->Hash == availableDeviceExtensionHash)
				{
					optionalDeviceExtensions.erase(optionalDeviceExtension);
					break;
				}
			}
		}

		requiredExtensionsSupported = requiredDeviceExtensions.empty();
		numOfOptionalExtensionsSupported = ARR_SIZE(OPTIONAL_DEVICE_EXTENSIONS) - (uint32)optionalDeviceExtensions.size();
	}

	GraphicsDeviceVK::QueueFamilyIndices GraphicsDeviceVK::FindQueueFamilies(VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		indices.GraphicsFamily = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilies);
		indices.ComputeFamily = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilies);
		indices.TransferFamily = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilies);
		indices.PresentFamily = indices.GraphicsFamily; //Assume present support at this stage

		return indices;
	}

	uint32 GraphicsDeviceVK::GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies)
	{
		if (queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
			{
				if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
					return i;
			}
		}

		if (queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
			{
				if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
					return i;
			}
		}

		for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
		{
			if (queueFamilies[i].queueFlags & queueFlags)
				return i;
		}

		return UINT32_MAX;
	}

	void GraphicsDeviceVK::SetEnabledDeviceExtensions()
	{
		//We know all requried device extensions are supported
		for (uint32 i = 0; i < ARR_SIZE(REQUIRED_DEVICE_EXTENSIONS); i++)
		{
			m_EnabledDeviceExtensions.push_back(REQUIRED_DEVICE_EXTENSIONS[i].Name);
		}

		std::vector<VkExtensionProperties> availableDeviceExtensions;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &extensionCount, nullptr);
		availableDeviceExtensions.resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data());

		std::vector<Extension> optionalDeviceExtensions(OPTIONAL_DEVICE_EXTENSIONS, OPTIONAL_DEVICE_EXTENSIONS + sizeof(OPTIONAL_DEVICE_EXTENSIONS) / sizeof(Extension));

		for (const VkExtensionProperties& extension : availableDeviceExtensions)
		{
			uint32 availableDeviceExtensionHash = HashString<const char*>(extension.extensionName);

			for (auto optionalDeviceExtension = optionalDeviceExtensions.begin(); optionalDeviceExtension != optionalDeviceExtensions.end(); optionalDeviceExtension++)
			{
				if (optionalDeviceExtension->Hash == availableDeviceExtensionHash)
				{
					m_EnabledDeviceExtensions.push_back(optionalDeviceExtension->Name);
					optionalDeviceExtensions.erase(optionalDeviceExtension);
					break;
				}
			}
		}

		if (optionalDeviceExtensions.size() > 0)
		{
			for (const Extension& optionalDeviceExtension : optionalDeviceExtensions)
			{
				LOG_WARNING("--- GraphicsDeviceVK: Optional Device Extension %s not supported", optionalDeviceExtension.Name);
			}
		}
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