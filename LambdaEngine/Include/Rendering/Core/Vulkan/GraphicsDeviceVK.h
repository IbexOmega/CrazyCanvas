#pragma once
#include <vector>
#include <optional>

#include "Utilities/StringHash.h"

#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/IGraphicsDevice.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class CommandBufferVK;
	
	typedef ConstString ValidationLayer;
	typedef ConstString Extension;

	struct QueueFamilyIndices
	{
		int32 GraphicsFamily	= -1;
		int32 ComputeFamily		= -1;
		int32 TransferFamily	= -1;
		int32 PresentFamily		= -1;

		bool IsComplete()
		{
			return (GraphicsFamily >= 0) && (ComputeFamily >= 0) && (TransferFamily >= 0) && (PresentFamily >= 0);
		}
	};

	class GraphicsDeviceVK : public IGraphicsDevice
	{
	public:
		GraphicsDeviceVK();
		~GraphicsDeviceVK();

		bool Init(const GraphicsDeviceDesc& desc);

		bool IsInstanceExtensionEnabled(const char* pExtensionName) const;
		bool IsDeviceExtensionEnabled(const char* pExtensionName)	const;

		void SetVulkanObjectName(const char* pName, uint64 objectHandle, VkObjectType type)	const;
		
		uint32						GetQueueFamilyIndexFromQueueType(ECommandQueueType type)	const;
		VkFormatProperties			GetFormatProperties(VkFormat format)						const;
		VkPhysicalDeviceProperties	GetPhysicalDeviceProperties()								const;

		FORCEINLINE QueueFamilyIndices GetQueueFamilyIndices() const
		{
			return m_DeviceQueueFamilyIndices;
		}

		//IGraphicsDevice Interface
		virtual IFrameBuffer*	CreateFrameBuffer(IRenderPass* pRenderPass, const FrameBufferDesc& desc)	const override final;
		virtual IRenderPass*	CreateRenderPass(const RenderPassDesc& desc)								const override final;
		virtual ITextureView*	CreateTextureView(const TextureViewDesc& desc)								const override final;

		virtual IBuffer*	CreateBuffer(const BufferDesc& desc)								const override final;
		virtual ITexture*	CreateTexture(const TextureDesc& desc)								const override final;
		virtual ISwapChain* CreateSwapChain(const Window* pWindow, const SwapChainDesc& desc)	const override final;

		virtual IPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineDesc& desc) 	  const override final;
		virtual IPipelineState* CreateComputePipelineState(const ComputePipelineDesc& desc) 	  const override final;
		virtual IPipelineState* CreateRayTracingPipelineState(const RayTracingPipelineDesc& desc) const override final;

		virtual ITopLevelAccelerationStructure*		CreateTopLevelAccelerationStructure(const TopLevelAccelerationStructureDesc& desc)			const override final;
		virtual IBottomLevelAccelerationStructure*	CreateBottomLevelAccelerationStructure(const BottomLevelAccelerationStructureDesc& desc)	const override final;

		virtual ICommandQueue*		CreateCommandQueue(const char* pName, ECommandQueueType queueType)				const override final;
		virtual ICommandAllocator*	CreateCommandAllocator(const char* pName, ECommandQueueType queueType)			const override final;
		virtual ICommandList*		CreateCommandList(ICommandAllocator* pAllocator, const CommandListDesc& desc)	const override final;
		virtual IFence*				CreateFence(const FenceDesc& desc)												const override final;
		
		virtual void Release() override final;

	private:
		//INIT
		bool InitInstance(const GraphicsDeviceDesc& desc);
		bool InitDevice(const GraphicsDeviceDesc& desc);
		bool InitPhysicalDevice();
		bool InitLogicalDevice(const GraphicsDeviceDesc& desc);

		//UTIL
		bool SetEnabledValidationLayers();
		bool SetEnabledInstanceExtensions();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		int32				RatePhysicalDevice(VkPhysicalDevice physicalDevice);
		void				CheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice, bool& requiredExtensionsSupported, uint32_t& numOfOptionalExtensionsSupported);
		QueueFamilyIndices	FindQueueFamilies(VkPhysicalDevice physicalDevice);
		
		uint32	GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const std::vector<VkQueueFamilyProperties>& queueFamilies);
		void	SetEnabledDeviceExtensions();

		void RegisterInstanceExtensionData();
		void RegisterDeviceExtensionData();

		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

	public:
		VkInstance			Instance		= VK_NULL_HANDLE;
		VkPhysicalDevice	PhysicalDevice	= VK_NULL_HANDLE;
		VkDevice			Device			= VK_NULL_HANDLE;

		//Extension Data
		VkPhysicalDeviceRayTracingPropertiesKHR	RayTracingProperties;

		PFN_vkSetDebugUtilsObjectNameEXT	vkSetDebugUtilsObjectNameEXT	= nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
		PFN_vkCreateDebugUtilsMessengerEXT	vkCreateDebugUtilsMessengerEXT	= nullptr;

		PFN_vkCreateAccelerationStructureKHR					vkCreateAccelerationStructureKHR				= nullptr;
		PFN_vkDestroyAccelerationStructureKHR					vkDestroyAccelerationStructureKHR				= nullptr;
		PFN_vkBindAccelerationStructureMemoryKHR				vkBindAccelerationStructureMemoryKHR			= nullptr;
		PFN_vkGetAccelerationStructureDeviceAddressKHR			vkGetAccelerationStructureDeviceAddressKHR		= nullptr;
		PFN_vkGetAccelerationStructureMemoryRequirementsKHR		vkGetAccelerationStructureMemoryRequirementsKHR = nullptr;
		PFN_vkCmdBuildAccelerationStructureKHR					vkCmdBuildAccelerationStructureKHR				= nullptr;
		PFN_vkCreateRayTracingPipelinesKHR						vkCreateRayTracingPipelinesKHR					= nullptr;
		PFN_vkGetRayTracingShaderGroupHandlesKHR				vkGetRayTracingShaderGroupHandlesKHR			= nullptr;
		PFN_vkCmdTraceRaysKHR									vkCmdTraceRaysKHR								= nullptr;

		//BufferAddresses
		PFN_vkGetBufferDeviceAddress	vkGetBufferDeviceAddress = nullptr;

		//Timeline-Semaphores
		PFN_vkWaitSemaphores vkWaitSemaphores						= nullptr;
		PFN_vkSignalSemaphore vkSignalSemaphore						= nullptr;
		PFN_vkGetSemaphoreCounterValue vkGetSemaphoreCounterValue	= nullptr;

	private:
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		QueueFamilyIndices		m_DeviceQueueFamilyIndices;
		VkPhysicalDeviceLimits	m_DeviceLimits;

		std::vector<const char*> m_EnabledValidationLayers;
		std::vector<const char*> m_EnabledInstanceExtensions;
		std::vector<const char*> m_EnabledDeviceExtensions;
	};
}
