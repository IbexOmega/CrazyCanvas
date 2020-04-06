#include <set>
#include <map>

#include "Log/Log.h"

#include "Rendering/Core/Vulkan/GraphicsPipelineStateVK.h"
#include "Rendering/Core/Vulkan/ComputePipelineStateVK.h"
#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/SwapChainVK.h"
#include "Rendering/Core/Vulkan/TopLevelAccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/BottomLevelAccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/CommandQueueVK.h"
#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"

#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	constexpr ValidationLayer REQUIRED_VALIDATION_LAYERS[]
	{
		ValidationLayer("REQ_V_L_BASE"),
		ValidationLayer("VK_LAYER_KHRONOS_validation")
	};

	constexpr ValidationLayer OPTIONAL_VALIDATION_LAYERS[]
	{
		ValidationLayer("OPT_V_L_BASE")
	};

	constexpr Extension REQUIRED_INSTANCE_EXTENSIONS[]
	{
		Extension("REQ_I_E_BASE"),
		Extension(VK_KHR_SURFACE_EXTENSION_NAME),
		Extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME),
#if defined(LAMBDA_PLATFORM_MACOS)
        Extension(VK_MVK_MACOS_SURFACE_EXTENSION_NAME),
#elif defined(LAMBDA_PLATFORM_WINDOWS)
		Extension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME),
#endif
    };

	constexpr Extension OPTIONAL_INSTANCE_EXTENSIONS[]
	{
		Extension("OPT_I_E_BASE"),
		Extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME),
	};

	constexpr Extension REQUIRED_DEVICE_EXTENSIONS[]
	{
		Extension("REQ_D_E_BASE"),
		Extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME),
	};

	constexpr Extension OPTIONAL_DEVICE_EXTENSIONS[]
	{
		Extension("OPT_D_E_BASE"),
		Extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME),
		Extension(VK_KHR_MAINTENANCE3_EXTENSION_NAME),
		Extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME),
		Extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME),
		Extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME),
		Extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME),
		Extension(VK_KHR_RAY_TRACING_EXTENSION_NAME),
	};

	GraphicsDeviceVK::GraphicsDeviceVK() :
		Instance(VK_NULL_HANDLE),
		Device(VK_NULL_HANDLE),
		PhysicalDevice(VK_NULL_HANDLE),
		vkSetDebugUtilsObjectNameEXT(nullptr),
		vkDestroyDebugUtilsMessengerEXT(nullptr),
		vkCreateDebugUtilsMessengerEXT(nullptr),
		m_DebugMessenger(VK_NULL_HANDLE),
		m_DeviceLimits()
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
			LOG_ERROR("[GraphicsDeviceVK]: Vulkan Instance could not be initialized!");
			return false;
		}
		else
		{
			LOG_MESSAGE("[GraphicsDeviceVK]: Vulkan Instance initialized!");
		}

		if (!InitDevice(desc))
		{
			LOG_ERROR("[GraphicsDeviceVK]: Vulkan Device could not be initialized!");
			return false;
		}
		else
		{
			LOG_MESSAGE("[GraphicsDeviceVK]: Vulkan Device initialized!");
		}

		return true;
	}

	void GraphicsDeviceVK::Release()
	{
		delete this;
	}

	IRenderPass* GraphicsDeviceVK::CreateRenderPass() const
	{
		return nullptr;
	}

	IPipelineState* GraphicsDeviceVK::CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) const
	{
		GraphicsPipelineStateVK* pPipelineState = DBG_NEW GraphicsPipelineStateVK(this);
		if (!pPipelineState->Init(desc))
		{
			pPipelineState->Release();
			return nullptr;
		}

		return pPipelineState;
	}

	IPipelineState* GraphicsDeviceVK::CreateComputePipelineState(const ComputePipelineDesc& desc) const
	{
		ComputePipelineStateVK* pPipelineState = DBG_NEW ComputePipelineStateVK(this);
		if (!pPipelineState->Init(desc))
		{
			pPipelineState->Release();
			return nullptr;
		}

		return pPipelineState;
	}

	IPipelineState* GraphicsDeviceVK::CreateRayTracingPipelineState(const RayTracingPipelineDesc& desc) const
	{
		RayTracingPipelineStateVK* pPipelineState = DBG_NEW RayTracingPipelineStateVK(this);
		if (!pPipelineState->Init(desc))
		{
			pPipelineState->Release();
			return nullptr;
		}

		return pPipelineState;
	}

	ITopLevelAccelerationStructure* GraphicsDeviceVK::CreateTopLevelAccelerationStructure(const TopLevelAccelerationStructureDesc& desc) const
	{
		TopLevelAccelerationStructureVK* pTLAS = DBG_NEW TopLevelAccelerationStructureVK(this);
		if (!pTLAS->Init(desc))
		{
			pTLAS->Release();
			return nullptr;
		}

		return pTLAS;
	}

	IBottomLevelAccelerationStructure* GraphicsDeviceVK::CreateBottomLevelAccelerationStructure(const BottomLevelAccelerationStructureDesc& desc) const
	{
		BottomLevelAccelerationStructureVK* pBLAS = DBG_NEW BottomLevelAccelerationStructureVK(this);
		if (!pBLAS->Init(desc))
		{
			pBLAS->Release();
			return nullptr;
		}

		return pBLAS;
	}

	ICommandList* GraphicsDeviceVK::CreateCommandList(ICommandAllocator* pAllocator, const CommandListDesc& desc) const
	{
        CommandListVK* pCommandListVK = DBG_NEW CommandListVK(this);
        if (!pCommandListVK->Init(pAllocator, desc))
        {
            pCommandListVK->Release();
            return nullptr;
        }
        
		return pCommandListVK;
	}

	ICommandAllocator* GraphicsDeviceVK::CreateCommandAllocator(ECommandQueueType queueType) const
	{
		CommandAllocatorVK* pCommandAllocator = DBG_NEW CommandAllocatorVK(this);
		if (!pCommandAllocator->Init(queueType))
		{
			pCommandAllocator->Release();
			return nullptr;
		}

		return pCommandAllocator;
	}

	ICommandQueue* GraphicsDeviceVK::CreateCommandQueue(ECommandQueueType queueType) const
	{
		int32 queueFamilyIndex = 0;
		if (queueType == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
		{
			queueFamilyIndex = m_DeviceQueueFamilyIndices.GraphicsFamily;
		}
		else if (queueType == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
		{
			queueFamilyIndex = m_DeviceQueueFamilyIndices.ComputeFamily;
		}
		else if (queueType == ECommandQueueType::COMMAND_QUEUE_COPY)
		{
			queueFamilyIndex = m_DeviceQueueFamilyIndices.TransferFamily;
		}
		else
		{
			return nullptr;
		}

		CommandQueueVK* pQueue = DBG_NEW CommandQueueVK(this);
		if (!pQueue->Init(queueFamilyIndex, 0))
		{
			pQueue->Release();
			return nullptr;
		}

		return pQueue;
	}

	IFence* GraphicsDeviceVK::CreateFence(const FenceDesc& desc) const
	{
		FenceVK* pFence = DBG_NEW FenceVK(this);
		if (!pFence->Init(desc))
		{
			pFence->Release();
			return nullptr;
		}

		return pFence;
	}

	IBuffer* GraphicsDeviceVK::CreateBuffer(const BufferDesc& desc) const
	{
		BufferVK* pBuffer = DBG_NEW BufferVK(this);
		if (!pBuffer->Init(desc))
		{
            pBuffer->Release();
			return nullptr;
		}

		return pBuffer;
	}

	ITexture* GraphicsDeviceVK::CreateTexture(const TextureDesc& desc) const
	{
		TextureVK* pTexture = DBG_NEW TextureVK(this);
		if (!pTexture->Init(desc))
		{
            pTexture->Release();
			return nullptr;
		}

		return pTexture;
	}

	ITextureView* GraphicsDeviceVK::CreateTextureView(const TextureViewDesc& desc) const
	{
        TextureViewVK* pTextureView = DBG_NEW TextureViewVK(this);
        if (!pTextureView->Init(desc))
        {
            pTextureView->Release();
            return nullptr;
        }
        
		return pTextureView;
	}

    ISwapChain* GraphicsDeviceVK::CreateSwapChain(const Window* pWindow, const SwapChainDesc& desc) const
    {
        SwapChainVK* pSwapChain = DBG_NEW SwapChainVK(this);
        if (!pSwapChain->Init(pWindow, desc))
        {
            pSwapChain->Release();
            return nullptr;
        }
        
        return pSwapChain;
    }

	void GraphicsDeviceVK::SetVulkanObjectName(const char* pName, uint64 objectHandle, VkObjectType type) const
	{
		if (pName)
		{
			if (vkSetDebugUtilsObjectNameEXT)
			{
				VkDebugUtilsObjectNameInfoEXT info = {};
				info.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
				info.pNext			= nullptr;
				info.objectType		= type;
				info.objectHandle	= objectHandle;
				info.pObjectName	= pName;
				vkSetDebugUtilsObjectNameEXT(Device, &info);
			}
		}
	}

	uint32 GraphicsDeviceVK::GetQueueFamilyIndexFromQueueType(ECommandQueueType type) const
	{
		if (type == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
		{
			return m_DeviceQueueFamilyIndices.GraphicsFamily;
		}
		else if (type == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
		{
			return m_DeviceQueueFamilyIndices.ComputeFamily;
		}
		else if (type == ECommandQueueType::COMMAND_QUEUE_COPY)
		{
			return m_DeviceQueueFamilyIndices.TransferFamily;
		}
		else if (type == ECommandQueueType::COMMAND_QUEUE_NONE)
		{
			return VK_QUEUE_FAMILY_IGNORED;
		}
		else
		{
			return 0xffffffff;
		}
	}

	VkFormatProperties GraphicsDeviceVK::GetFormatProperties(VkFormat format) const
	{
		VkFormatProperties properties = {};
		vkGetPhysicalDeviceFormatProperties(PhysicalDevice, format, &properties);

		return properties;
	}

	bool GraphicsDeviceVK::InitInstance(const GraphicsDeviceDesc& desc)
	{
		if (desc.Debug)
		{
			if (!SetEnabledValidationLayers())
			{
				LOG_ERROR("[GraphicsDeviceVK]: Validation Layers not supported");
				return false;
			}
		}

		if (!SetEnabledInstanceExtensions())
		{
			LOG_ERROR("[GraphicsDeviceVK]: Required Instance Extensions not supported");
			return false;
		}

		//USE API VERSION 1.2 for now, maybe change to 1.0 later
		VkApplicationInfo appInfo = {};
		appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext               = nullptr;
		appInfo.pApplicationName    = "Lambda Engine";
		appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName         = "Lambda Engine";
		appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion          = VK_API_VERSION_1_2;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo         = &appInfo;
		instanceCreateInfo.enabledExtensionCount    = (uint32_t)m_EnabledInstanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames  = m_EnabledInstanceExtensions.data();

		if (desc.Debug)
		{
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
			PopulateDebugMessengerCreateInfo(debugCreateInfo);

			instanceCreateInfo.enabledLayerCount    = (uint32_t)m_EnabledValidationLayers.size();
			instanceCreateInfo.ppEnabledLayerNames  = m_EnabledValidationLayers.data();
			instanceCreateInfo.pNext                = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			instanceCreateInfo.enabledLayerCount    = 0;
			instanceCreateInfo.pNext                = nullptr;
		}

		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &Instance);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[GraphicsDeviceVK]: Failed to create Vulkan Instance!", result);
			return false;
		}

		RegisterInstanceExtensionData();

		if (desc.Debug)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			PopulateDebugMessengerCreateInfo(createInfo);

			result = vkCreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &m_DebugMessenger);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR("[GraphicsDeviceVK]: Failed to set up Debug Messenger!", result);
				return false;
			}
		}

		return true;
	}

	bool GraphicsDeviceVK::InitDevice(const GraphicsDeviceDesc& desc)
	{
		if (!InitPhysicalDevice())
		{
			LOG_ERROR("[GraphicsDeviceVK]: Could not initialize Physical Device!");
			return false;
		}

		if (!InitLogicalDevice(desc))
		{
			LOG_ERROR("[GraphicsDeviceVK]: Could not initialize Logical Device!");
			return false;
		}

		RegisterDeviceExtensionData();
		return true;
	}

	bool GraphicsDeviceVK::InitPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			LOG_ERROR("[GraphicsDeviceVK]: Presentation is not supported by the selected physicaldevice");
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
			LOG_ERROR("[GraphicsDeviceVK]: Failed to find a suitable GPU!");
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
		std::set<int32> uniqueQueueFamilies =
		{
			m_DeviceQueueFamilyIndices.GraphicsFamily,
			m_DeviceQueueFamilyIndices.ComputeFamily,
			m_DeviceQueueFamilyIndices.TransferFamily,
			m_DeviceQueueFamilyIndices.PresentFamily
		};

		float queuePriority = 1.0f;
		for (int32 queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex    = uint32(queueFamily);
			queueCreateInfo.queueCount          = 1;
			queueCreateInfo.pQueuePriorities    = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceVulkan12Features deviceFeatures12 = {};
		deviceFeatures12.sType					= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		deviceFeatures12.pNext					= nullptr;
		deviceFeatures12.bufferDeviceAddress	= true;
		deviceFeatures12.timelineSemaphore		= true;

		VkPhysicalDeviceVulkan11Features deviceFeatures11 = {};
		deviceFeatures11.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		deviceFeatures11.pNext	= &deviceFeatures12;

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.fillModeNonSolid					= true;
		deviceFeatures.vertexPipelineStoresAndAtomics	= true;
		deviceFeatures.fragmentStoresAndAtomics			= true;

		VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.sType	 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext	 = &deviceFeatures11;
		deviceFeatures2.features = deviceFeatures;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext					= &deviceFeatures2;
		createInfo.flags					= 0;
		createInfo.queueCreateInfoCount		= (uint32)queueCreateInfos.size();
		createInfo.pQueueCreateInfos		= queueCreateInfos.data();
		createInfo.enabledExtensionCount    = (uint32)m_EnabledDeviceExtensions.size();
		createInfo.ppEnabledExtensionNames  = m_EnabledDeviceExtensions.data();

		if (desc.Debug)
		{
			createInfo.enabledLayerCount    = (uint32)m_EnabledValidationLayers.size();
			createInfo.ppEnabledLayerNames  = m_EnabledValidationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VkResult result = vkCreateDevice(PhysicalDevice, &createInfo, nullptr, &Device);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[GraphicsDeviceVK]: Failed to create logical device!", result);
			return false;
		}

		return true;
	}

	bool GraphicsDeviceVK::SetEnabledValidationLayers()
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableValidationLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableValidationLayers.data());

		std::vector<ValidationLayer> requiredValidationLayers(REQUIRED_VALIDATION_LAYERS + 1, REQUIRED_VALIDATION_LAYERS + sizeof(REQUIRED_VALIDATION_LAYERS) / sizeof(ValidationLayer));
		std::vector<ValidationLayer> optionalValidationLayers(OPTIONAL_VALIDATION_LAYERS + 1, OPTIONAL_VALIDATION_LAYERS + sizeof(OPTIONAL_VALIDATION_LAYERS) / sizeof(ValidationLayer));

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
				LOG_ERROR("[GraphicsDeviceVK]: Required Validation Layer %s not supported", requiredValidationLayer.Name);
			}

			return false;
		}

		if (optionalValidationLayers.size() > 0)
		{
			for (const ValidationLayer& optionalValidationLayer : optionalValidationLayers)
			{
				LOG_WARNING("[GraphicsDeviceVK]: Optional Validation Layer %s not supported", optionalValidationLayer.Name);
			}
		}

		return true;
	}

	bool GraphicsDeviceVK::SetEnabledInstanceExtensions()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableInstanceExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data());

		std::vector<Extension> requiredInstanceExtensions(REQUIRED_INSTANCE_EXTENSIONS + 1, REQUIRED_INSTANCE_EXTENSIONS + sizeof(REQUIRED_INSTANCE_EXTENSIONS) / sizeof(Extension));
		std::vector<Extension> optionalInstanceExtensions(OPTIONAL_INSTANCE_EXTENSIONS + 1, OPTIONAL_INSTANCE_EXTENSIONS + sizeof(OPTIONAL_INSTANCE_EXTENSIONS) / sizeof(Extension));

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
				LOG_ERROR("[GraphicsDeviceVK]: Required Instance Extension %s not supported", requiredInstanceExtension.Name);
			}

			return false;
		}

		if (optionalInstanceExtensions.size() > 0)
		{
			for (const Extension& optionalInstanceExtension : optionalInstanceExtensions)
			{
				LOG_WARNING("[GraphicsDeviceVK]: Optional Instance Extension %s not supported", optionalInstanceExtension.Name);
			}
		}

		return true;
	}

	void GraphicsDeviceVK::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity	= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType		= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback	= DebugCallback;
		createInfo.pUserData		= nullptr;
	}

	int32 GraphicsDeviceVK::RatePhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		bool        requiredExtensionsSupported			= false;
		uint32_t    numOfOptionalExtensionsSupported	= 0;
		CheckDeviceExtensionsSupport(physicalDevice, requiredExtensionsSupported, numOfOptionalExtensionsSupported);

		if (!requiredExtensionsSupported)
        {
            return 0;
        }

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

		if (!indices.IsComplete())
        {
            return 0;
        }

		int score = 1 + numOfOptionalExtensionsSupported;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

		return score;
	}

	void GraphicsDeviceVK::CheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice, bool& requiredExtensionsSupported, uint32_t& numOfOptionalExtensionsSupported)
	{
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableDeviceExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data());

		std::vector<Extension> requiredDeviceExtensions(REQUIRED_DEVICE_EXTENSIONS + 1, REQUIRED_DEVICE_EXTENSIONS + sizeof(REQUIRED_DEVICE_EXTENSIONS) / sizeof(Extension));
		std::vector<Extension> optionalDeviceExtensions(OPTIONAL_DEVICE_EXTENSIONS + 1, OPTIONAL_DEVICE_EXTENSIONS + sizeof(OPTIONAL_DEVICE_EXTENSIONS) / sizeof(Extension));

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

		requiredExtensionsSupported			= requiredDeviceExtensions.empty();
		numOfOptionalExtensionsSupported	= ARR_SIZE(OPTIONAL_DEVICE_EXTENSIONS) - (uint32)optionalDeviceExtensions.size();
	}

	QueueFamilyIndices GraphicsDeviceVK::FindQueueFamilies(VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices = {};

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		indices.GraphicsFamily  = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilies);
		indices.ComputeFamily   = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilies);
		indices.TransferFamily  = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilies);
		indices.PresentFamily   = indices.GraphicsFamily; //Assume present support at this stage

		return indices;
	}

	uint32 GraphicsDeviceVK::GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies)
	{
		if (queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
			{
				if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    return i;
                }
			}
		}

		if (queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
			{
				if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    return i;
                }
			}
		}

		for (uint32_t i = 0; i < uint32_t(queueFamilies.size()); i++)
		{
			if (queueFamilies[i].queueFlags & queueFlags)
            {
                return i;
            }
		}

		return UINT32_MAX;
	}

	void GraphicsDeviceVK::SetEnabledDeviceExtensions()
	{
		//We know all requried device extensions are supported
		for (uint32 i = 1; i < ARR_SIZE(REQUIRED_DEVICE_EXTENSIONS); i++)
		{
			m_EnabledDeviceExtensions.push_back(REQUIRED_DEVICE_EXTENSIONS[i].Name);
		}

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableDeviceExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data());

		std::vector<Extension> optionalDeviceExtensions(OPTIONAL_DEVICE_EXTENSIONS + 1, OPTIONAL_DEVICE_EXTENSIONS + sizeof(OPTIONAL_DEVICE_EXTENSIONS) / sizeof(Extension));
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
				LOG_WARNING("[GraphicsDeviceVK]: Optional Device Extension %s not supported", optionalDeviceExtension.Name);
			}
		}
	}

	bool GraphicsDeviceVK::IsInstanceExtensionEnabled(const char* pExtensionName)
	{
		uint32 extensionHash = HashString<const char*>(pExtensionName);
		for (const char* pEnabledExtensionName : m_EnabledInstanceExtensions)
		{
			uint32 enabledExtensionHash = HashString<const char*>(pEnabledExtensionName);
			if (extensionHash == enabledExtensionHash)
			{
				return true;
			}
		}

		return false;
	}

	bool GraphicsDeviceVK::IsDeviceExtensionEnabled(const char* pExtensionName)
	{
		uint32 extensionHash = HashString<const char*>(pExtensionName);
		for (const char* pEnabledExtensionName : m_EnabledDeviceExtensions)
		{
			uint32 enabledExtensionHash = HashString<const char*>(pEnabledExtensionName);
			if (extensionHash == enabledExtensionHash)
			{
				return true;
			}
		}

		return false;
	}

	void GraphicsDeviceVK::RegisterInstanceExtensionData()
	{
		//Required
		{
			GET_INSTANCE_PROC_ADDR(Instance, vkCreateDebugUtilsMessengerEXT);
			GET_INSTANCE_PROC_ADDR(Instance, vkDestroyDebugUtilsMessengerEXT);
			GET_INSTANCE_PROC_ADDR(Instance, vkSetDebugUtilsObjectNameEXT);
		}
	}

	void GraphicsDeviceVK::RegisterDeviceExtensionData()
	{
		if (IsDeviceExtensionEnabled(VK_KHR_RAY_TRACING_EXTENSION_NAME))
		{
			GET_DEVICE_PROC_ADDR(Device, vkCreateAccelerationStructureKHR);
			GET_DEVICE_PROC_ADDR(Device, vkDestroyAccelerationStructureKHR);
			GET_DEVICE_PROC_ADDR(Device, vkBindAccelerationStructureMemoryKHR);
			GET_DEVICE_PROC_ADDR(Device, vkGetAccelerationStructureDeviceAddressKHR);
			GET_DEVICE_PROC_ADDR(Device, vkGetAccelerationStructureMemoryRequirementsKHR);
			GET_DEVICE_PROC_ADDR(Device, vkCmdBuildAccelerationStructureKHR);
			GET_DEVICE_PROC_ADDR(Device, vkCreateRayTracingPipelinesKHR);
			GET_DEVICE_PROC_ADDR(Device, vkGetRayTracingShaderGroupHandlesKHR);
			GET_DEVICE_PROC_ADDR(Device, vkCmdTraceRaysKHR);

			//Query Ray Tracing properties
			RayTracingProperties = {};
			RayTracingProperties.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
			
			VkPhysicalDeviceProperties2 deviceProps2 = {};
			deviceProps2.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProps2.pNext	= &RayTracingProperties;

			vkGetPhysicalDeviceProperties2(PhysicalDevice, &deviceProps2);
		}

		//TOOO: Check for extension
		GET_DEVICE_PROC_ADDR(Device, vkGetBufferDeviceAddress);

		GET_DEVICE_PROC_ADDR(Device, vkWaitSemaphores);
		GET_DEVICE_PROC_ADDR(Device, vkSignalSemaphore);
		GET_DEVICE_PROC_ADDR(Device, vkGetSemaphoreCounterValue);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsDeviceVK::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		UNREFERENCED_VARIABLE(messageType);
		UNREFERENCED_VARIABLE(pCallbackData);
		UNREFERENCED_VARIABLE(pUserData);

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			LOG_MESSAGE("[Validation Layer]: %s", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			LOG_MESSAGE("[Validation Layer]: %s", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LOG_WARNING("[Validation Layer]: %s", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LOG_ERROR("[Validation Layer]: %s", pCallbackData->pMessage);
		}

		return VK_FALSE;
	}
}
