#pragma once
#include "Containers/TArray.h"

#include "Utilities/StringHash.h"

#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/GraphicsDeviceBase.h"

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
		int32 PresentFamily		= -1;

		bool IsComplete()
		{
			return (GraphicsFamily >= 0) && (ComputeFamily >= 0) && (TransferFamily >= 0) && (PresentFamily >= 0);
		}
	};

	class GraphicsDeviceVK final : public GraphicsDeviceBase
	{
	public:
		GraphicsDeviceVK();
		~GraphicsDeviceVK();

		bool Init(const GraphicsDeviceDesc* pDesc);

        VkResult    AllocateMemory(VkDeviceMemory* pDeviceMemory, VkDeviceSize sizeInBytes, int32 memoryIndex)  const;
        void        FreeMemory(VkDeviceMemory deviceMemory)                                                     const;
        
        void DestroyRenderPass(VkRenderPass* pRenderPass)   const;
        void DestroyImageView(VkImageView* pImageView)      const;
		
		bool IsInstanceExtensionEnabled(const char* pExtensionName) const;
		bool IsDeviceExtensionEnabled(const char* pExtensionName)	const;

		void SetVulkanObjectName(const char* pName, uint64 objectHandle, VkObjectType type)	const;
		
        VkFramebuffer GetFrameBuffer(const IRenderPass* pRenderPass, const ITextureView* const* ppRenderTargets, uint32 renderTargetCount, const ITextureView* pDepthStencil, uint32 width, uint32 height) const;
        
		uint32						GetQueueFamilyIndexFromQueueType(ECommandQueueType type)	const;
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

		// IGraphicsDevice Interface
		virtual IPipelineLayout* CreatePipelineLayout(const PipelineLayoutDesc* pDesc) const override final;
		virtual IDescriptorHeap* CreateDescriptorHeap(const DescriptorHeapDesc* pDesc) const override final;

		virtual IDescriptorSet* CreateDescriptorSet(const char* pName, const IPipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, IDescriptorHeap* pDescriptorHeap) const override final;

		virtual IRenderPass*	CreateRenderPass(const RenderPassDesc* pDesc)	const override final;
		virtual ITextureView*	CreateTextureView(const TextureViewDesc* pDesc)	const override final;

		virtual IShader*	CreateShader(const ShaderDesc* pDesc)	const override final;

		virtual IBuffer*	CreateBuffer(const BufferDesc* pDesc, IDeviceAllocator* pAllocator)	    const override final;
		virtual ITexture*	CreateTexture(const TextureDesc* pDesc, IDeviceAllocator* pAllocator)	const override final;
		virtual ISampler*	CreateSampler(const SamplerDesc* pDesc)	                                const override final;

		virtual ISwapChain* CreateSwapChain(const Window* pWindow, ICommandQueue* pCommandQueue, const SwapChainDesc* pDesc)	const override final;

		virtual IPipelineState* CreateGraphicsPipelineState(const GraphicsPipelineStateDesc* pDesc) 	  const override final;
		virtual IPipelineState* CreateComputePipelineState(const ComputePipelineStateDesc* pDesc) 	  const override final;
		virtual IPipelineState* CreateRayTracingPipelineState(const RayTracingPipelineStateDesc* pDesc) const override final;

		virtual IAccelerationStructure* CreateAccelerationStructure(const AccelerationStructureDesc* pDesc, IDeviceAllocator* pAllocator) const override final;

		virtual ICommandQueue*		CreateCommandQueue(const char* pName, ECommandQueueType queueType)				const override final;
		virtual ICommandAllocator*	CreateCommandAllocator(const char* pName, ECommandQueueType queueType)			const override final;
		virtual ICommandList*		CreateCommandList(ICommandAllocator* pAllocator, const CommandListDesc* pDesc)	const override final;
		virtual IFence*				CreateFence(const FenceDesc* pDesc)												const override final;

        virtual IDeviceAllocator* CreateDeviceAllocator(const DeviceAllocatorDesc* pDesc) const override final;
        
		virtual void CopyDescriptorSet(const IDescriptorSet* pSrc, IDescriptorSet* pDst)																			const override final;
		virtual void CopyDescriptorSet(const IDescriptorSet* pSrc, IDescriptorSet* pDst, const CopyDescriptorBindingDesc* pCopyBindings, uint32 copyBindingCount)	const override final;

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
		PFN_vkWaitSemaphores			vkWaitSemaphores			= nullptr;
		PFN_vkSignalSemaphore			vkSignalSemaphore			= nullptr;
		PFN_vkGetSemaphoreCounterValue	vkGetSemaphoreCounterValue	= nullptr;

	private:
		VkDebugUtilsMessengerEXT	m_DebugMessenger	= VK_NULL_HANDLE;
		FrameBufferCacheVK*			m_pFrameBufferCache	= nullptr;

        QueueFamilyIndices     m_DeviceQueueFamilyIndices;
        VkPhysicalDeviceLimits m_DeviceLimits;
       
        std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
        mutable uint32 m_NextGraphicsQueue  = 0;
        mutable uint32 m_NextComputeQueue   = 0;
        mutable uint32 m_NextTransferQueue  = 0;
        
        mutable uint32 m_UsedAllocations = 0;
        
		std::vector<const char*>	m_EnabledValidationLayers;
		std::vector<const char*>	m_EnabledInstanceExtensions;
		std::vector<const char*>	m_EnabledDeviceExtensions;
	};
}
