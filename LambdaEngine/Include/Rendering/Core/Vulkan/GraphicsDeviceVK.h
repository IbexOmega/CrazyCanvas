#pragma once
#include "Containers/TArray.h"

#include "Utilities/StringHash.h"

#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/GraphicsDevice.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class CommandBufferVK;
	class FrameBufferCacheVK;
	
	typedef ConstString ValidationLayer;
	typedef ConstString Extension;

	struct QueueFamilyIndices
	{
		int32 GraphicsFamily	= -1;
		int32 ComputeFamily		= -1;
		int32 TransferFamily	= -1;

		bool IsComplete()
		{
			return (GraphicsFamily >= 0) && (ComputeFamily >= 0) && (TransferFamily >= 0);
		}
	};

	class GraphicsDeviceVK final : public GraphicsDevice
	{
	public:
		GraphicsDeviceVK();
		~GraphicsDeviceVK();

		bool Init(const GraphicsDeviceDesc* pDesc);

		VkResult	AllocateMemory(VkDeviceMemory* pDeviceMemory, VkDeviceSize sizeInBytes, int32 memoryIndex)	const;
		void		FreeMemory(VkDeviceMemory deviceMemory)														const;
		
		void DestroyRenderPass(VkRenderPass* pRenderPass)	const;
		void DestroyImageView(VkImageView* pImageView)		const;
		
		bool IsInstanceExtensionEnabled(const char* pExtensionName) const;
		bool IsDeviceExtensionEnabled(const char* pExtensionName)	const;
		bool UseTimelineFences() const;

		void SetVulkanObjectName(const String& debugname, uint64 objectHandle, VkObjectType type)	const;
		
		VkFramebuffer GetFrameBuffer(const RenderPass* pRenderPass, const TextureView* const* ppRenderTargets, uint32 renderTargetCount, const TextureView* pDepthStencil, uint32 width, uint32 height) const;
		
		uint32						GetQueueFamilyIndexFromQueueType(ECommandQueueType type)	const;
		ECommandQueueType			GetCommandQueueTypeFromQueueIndex(uint32 queueFamilyIndex)	const;
		VkFormatProperties			GetFormatProperties(VkFormat format)						const;
		VkPhysicalDeviceProperties	GetPhysicalDeviceProperties()								const;
		
		FORCEINLINE VkPhysicalDeviceLimits GetDeviceLimits() const
		{
			return m_DeviceLimits;
		}

		FORCEINLINE QueueFamilyIndices GetQueueFamilyIndices() const
		{
			return m_DeviceQueueFamilyIndices;
		}

	public:
		// GraphicsDevice Interface
		virtual QueryHeap* CreateQueryHeap(const QueryHeapDesc* pDesc) const override final;

		virtual PipelineLayout* CreatePipelineLayout(const PipelineLayoutDesc* pDesc) const override final;
		virtual DescriptorHeap* CreateDescriptorHeap(const DescriptorHeapDesc* pDesc) const override final;

		virtual DescriptorSet* CreateDescriptorSet(const String& debugName, const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, DescriptorHeap* pDescriptorHeap) const override final;

		virtual RenderPass*		CreateRenderPass(const RenderPassDesc* pDesc)	const override final;
		virtual TextureView*	CreateTextureView(const TextureViewDesc* pDesc)	const override final;

		virtual Shader*	CreateShader(const ShaderDesc* pDesc)	const override final;

		virtual Buffer*		CreateBuffer(const BufferDesc* pDesc, DeviceAllocator* pAllocator)		const override final;
		virtual Texture*	CreateTexture(const TextureDesc* pDesc, DeviceAllocator* pAllocator)	const override final;
		virtual Sampler*	CreateSampler(const SamplerDesc* pDesc)									const override final;

		virtual SwapChain* CreateSwapChain(const SwapChainDesc* pDesc)	const override final;

		virtual PipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateDesc* pDesc) 	  const override final;
		virtual PipelineState* CreateComputePipelineState(const ComputePipelineStateDesc* pDesc) 	  const override final;
		virtual PipelineState* CreateRayTracingPipelineState(CommandQueue* pCommandQueue, const RayTracingPipelineStateDesc* pDesc) const override final;

		virtual AccelerationStructure* CreateAccelerationStructure(const AccelerationStructureDesc* pDesc, DeviceAllocator* pAllocator) const override final;

		virtual CommandQueue*		CreateCommandQueue(const String& debugname, ECommandQueueType queueType)		const override final;
		virtual CommandAllocator*	CreateCommandAllocator(const String& debugname, ECommandQueueType queueType)	const override final;
		virtual CommandList*		CreateCommandList(CommandAllocator* pAllocator, const CommandListDesc* pDesc)	const override final;
		virtual Fence*				CreateFence(const FenceDesc* pDesc)												const override final;

		virtual DeviceAllocator* CreateDeviceAllocator(const DeviceAllocatorDesc* pDesc) const override final;
		
		virtual void CopyDescriptorSet(const DescriptorSet* pSrc, DescriptorSet* pDst)																			const override final;
		virtual void CopyDescriptorSet(const DescriptorSet* pSrc, DescriptorSet* pDst, const CopyDescriptorBindingDesc* pCopyBindings, uint32 copyBindingCount)	const override final;

		virtual void QueryDeviceFeatures(GraphicsDeviceFeatureDesc* pFeatures) const override final;

		virtual void Release() override final;

	private:
		bool InitInstance(const GraphicsDeviceDesc* pDesc);
		bool InitDevice(const GraphicsDeviceDesc* pDesc);
		bool InitPhysicalDevice();
		bool InitLogicalDevice(const GraphicsDeviceDesc* pDesc);

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

	public:
		/*
		* Extension Data
		*/

		// Ray-Tracing
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

		// Buffer Addresses
		PFN_vkGetBufferDeviceAddress	vkGetBufferDeviceAddress = nullptr;

		// Timeline-Semaphores
		PFN_vkWaitSemaphores			vkWaitSemaphores			= nullptr;
		PFN_vkSignalSemaphore			vkSignalSemaphore			= nullptr;
		PFN_vkGetSemaphoreCounterValue	vkGetSemaphoreCounterValue	= nullptr;
	
	private:
		VkDebugUtilsMessengerEXT	m_DebugMessenger	= VK_NULL_HANDLE;
		FrameBufferCacheVK*			m_pFrameBufferCache	= nullptr;

		GraphicsDeviceFeatureDesc		m_DeviceFeatures;
		QueueFamilyIndices				m_DeviceQueueFamilyIndices;
		VkPhysicalDeviceLimits			m_DeviceLimits;
		VkPhysicalDeviceFeatures		m_DeviceFeaturesVk;
	   
		TArray<VkQueueFamilyProperties> m_QueueFamilyProperties;
		mutable uint32 m_NextGraphicsQueue	= 0;
		mutable uint32 m_NextComputeQueue	= 0;
		mutable uint32 m_NextTransferQueue	= 0;
		mutable uint32 m_UsedAllocations	= 0;
		
		TArray<const char*>	m_EnabledValidationLayers;
		TArray<const char*>	m_EnabledInstanceExtensions;
		TArray<const char*>	m_EnabledDeviceExtensions;
	};
}
