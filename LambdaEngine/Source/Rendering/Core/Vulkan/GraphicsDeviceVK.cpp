#include <set>
#include <map>

#include "Log/Log.h"

#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/FenceLegacyVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/CommandQueueVK.h"
#include "Rendering/Core/Vulkan/GraphicsPipelineStateVK.h"
#include "Rendering/Core/Vulkan/ComputePipelineStateVK.h"
#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/SwapChainVK.h"
#include "Rendering/Core/Vulkan/AccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/RenderPassVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/DescriptorHeapVK.h"
#include "Rendering/Core/Vulkan/DescriptorSetVK.h"
#include "Rendering/Core/Vulkan/FrameBufferCacheVK.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
    /*
     * ValidationLayers and Extensions
     */

	constexpr ValidationLayer REQUIRED_VALIDATION_LAYERS[]
	{
		ValidationLayer("REQ_V_L_BASE"),
		ValidationLayer("VK_LAYER_KHRONOS_validation"),
		//ValidationLayer("VK_LAYER_RENDERDOC_Capture")
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
		Extension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME),
        Extension("VK_KHR_shader_draw_parameters"),
	};

    /*
     * GraphicsDeviceVK
     */

	GraphicsDeviceVK::GraphicsDeviceVK()
        : IGraphicsDevice(),
        RayTracingProperties(),
        m_DeviceQueueFamilyIndices(),
        m_DeviceLimits(),
        m_QueueFamilyProperties(),
        m_EnabledValidationLayers(),
        m_EnabledInstanceExtensions(),
        m_EnabledDeviceExtensions()
	{
	}

	GraphicsDeviceVK::~GraphicsDeviceVK()
	{
		SAFEDELETE(m_pFrameBufferCache);
		
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

	bool GraphicsDeviceVK::Init(const GraphicsDeviceDesc* pDesc)
	{
        VALIDATE(pDesc != nullptr);
        
		if (!InitInstance(pDesc))
		{
			LOG_ERROR("[GraphicsDeviceVK]: Vulkan Instance could not be initialized!");
			return false;
		}
		else
		{
			LOG_MESSAGE("[GraphicsDeviceVK]: Vulkan Instance initialized!");
		}

		if (!InitDevice(pDesc))
		{
			LOG_ERROR("[GraphicsDeviceVK]: Vulkan Device could not be initialized!");
			return false;
		}
		else
		{
			LOG_MESSAGE("[GraphicsDeviceVK]: Vulkan Device initialized!");
		}

		m_pFrameBufferCache = DBG_NEW FrameBufferCacheVK(this);

		return true;
	}

    VkResult GraphicsDeviceVK::AllocateMemory(VkDeviceMemory* pDeviceMemory, VkDeviceSize sizeInBytes, int32 memoryIndex) const
    {
        VALIDATE(pDeviceMemory      !=  nullptr);
        VALIDATE(m_UsedAllocations  <   m_DeviceLimits.maxMemoryAllocationCount);

        VkMemoryAllocateFlagsInfo allocateFlagsInfo = {};
        allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        VkMemoryAllocateInfo allocateInfo = { };
        allocateInfo.sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext              = &allocateFlagsInfo;
        allocateInfo.memoryTypeIndex    = memoryIndex;
        allocateInfo.allocationSize     = sizeInBytes;

        VkResult result = vkAllocateMemory(Device, &allocateInfo, nullptr, pDeviceMemory);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[GraphicsDeviceVK]: Failed to allocate memory");
        }
        else
        {
            m_UsedAllocations++;
            D_LOG_INFO("[GraphicsDeviceVK]: Allocated %u bytes. Allocations %u/%u", sizeInBytes, m_UsedAllocations, m_DeviceLimits.maxMemoryAllocationCount);
        }
        
        return result;
    }

    void GraphicsDeviceVK::FreeMemory(VkDeviceMemory deviceMemory) const
    {
        VALIDATE(deviceMemory != VK_NULL_HANDLE);
        vkFreeMemory(Device, deviceMemory, nullptr);
        m_UsedAllocations--;
    }

    void GraphicsDeviceVK::DestroyRenderPass(VkRenderPass* pRenderPass) const
    {
        ASSERT(m_pFrameBufferCache != nullptr);
        
        if (*pRenderPass != VK_NULL_HANDLE)
        {
            m_pFrameBufferCache->DestroyRenderPass(*pRenderPass);
            
            vkDestroyRenderPass(Device, *pRenderPass, nullptr);
            *pRenderPass = VK_NULL_HANDLE;
        }
    }

    void GraphicsDeviceVK::DestroyImageView(VkImageView* pImageView) const
    {
        ASSERT(m_pFrameBufferCache != nullptr);
        
        if (*pImageView != VK_NULL_HANDLE)
        {
            m_pFrameBufferCache->DestroyImageView(*pImageView);
            
            vkDestroyImageView(Device, *pImageView, nullptr);
            *pImageView = VK_NULL_HANDLE;
        }
    }

	VkFramebuffer GraphicsDeviceVK::GetFrameBuffer(const IRenderPass* pRenderPass, const ITextureView* const* ppRenderTargets, uint32 renderTargetCount, const ITextureView* pDepthStencil, uint32 width, uint32 height) const
	{
		FrameBufferCacheKey key = { };
		for (uint32 i = 0; i < renderTargetCount; i++)
		{
            const TextureViewVK* pRenderTargetVk = reinterpret_cast<const TextureViewVK*>(ppRenderTargets[i]);
			key.ColorAttachmentsViews[i] = pRenderTargetVk->GetImageView();
		}

		key.ColorAttachmentViewCount	= renderTargetCount;
        
        if (pDepthStencil)
        {
            const TextureViewVK* pDepthStencilVk = reinterpret_cast<const TextureViewVK*>(pDepthStencil);
            key.DepthStencilView = pDepthStencilVk->GetImageView();
        }
        else
        {
            key.DepthStencilView = VK_NULL_HANDLE;
        }
        
        VALIDATE(pRenderPass != nullptr);
        
        const RenderPassVK* pRenderPassVk = reinterpret_cast<const RenderPassVK*>(pRenderPass);
        key.RenderPass = pRenderPassVk->GetRenderPass();
		
		return m_pFrameBufferCache->GetFrameBuffer(key, width, height);
	}

    /*
     * Release
     */

	void GraphicsDeviceVK::Release()
	{
		delete this;
	}

    /*
     * Create functions
     */

	IPipelineLayout* GraphicsDeviceVK::CreatePipelineLayout(const PipelineLayoutDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
		PipelineLayoutVK* pPipelineLayout = DBG_NEW PipelineLayoutVK(this);
		if (!pPipelineLayout->Init(pDesc))
		{
			pPipelineLayout->Release();
			return nullptr;
		}
		else
		{
			return pPipelineLayout;
		}
	}

	IDescriptorHeap* GraphicsDeviceVK::CreateDescriptorHeap(const DescriptorHeapDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
		DescriptorHeapVK* pDescriptorHeap = DBG_NEW DescriptorHeapVK(this);
		if (!pDescriptorHeap->Init(pDesc))
		{
			pDescriptorHeap->Release();
			return nullptr;
		}
		else
		{
			return pDescriptorHeap;
		}
	}

	IDescriptorSet* GraphicsDeviceVK::CreateDescriptorSet(const char* pName, const IPipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, IDescriptorHeap* pDescriptorHeap) const
	{
        VALIDATE(pPipelineLayout != nullptr);
        VALIDATE(pDescriptorHeap != nullptr);
        
		DescriptorSetVK* pDescriptorSet = DBG_NEW DescriptorSetVK(this);
		if (!pDescriptorSet->Init(pName, pPipelineLayout, descriptorLayoutIndex, pDescriptorHeap))
		{
			pDescriptorSet->Release();
			return nullptr;
		}
		else
		{
			return pDescriptorSet;
		}
	}

	IRenderPass* GraphicsDeviceVK::CreateRenderPass(const RenderPassDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
 		RenderPassVK* pRenderPass = DBG_NEW RenderPassVK(this);
		if (!pRenderPass->Init(pDesc))
		{
			pRenderPass->Release();
			return nullptr;
		}
		else
		{
			return pRenderPass;
		}
	}

	IPipelineState* GraphicsDeviceVK::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
		GraphicsPipelineStateVK* pPipelineState = DBG_NEW GraphicsPipelineStateVK(this);
		if (!pPipelineState->Init(pDesc))
		{
			pPipelineState->Release();
			return nullptr;
		}
		else
		{
			return pPipelineState;
		}
	}

	IPipelineState* GraphicsDeviceVK::CreateComputePipelineState(const ComputePipelineStateDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
		ComputePipelineStateVK* pPipelineState = DBG_NEW ComputePipelineStateVK(this);
		if (!pPipelineState->Init(pDesc))
		{
			pPipelineState->Release();
			return nullptr;
		}
		else
		{
			return pPipelineState;
		}
	}

	IPipelineState* GraphicsDeviceVK::CreateRayTracingPipelineState(const RayTracingPipelineStateDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
		RayTracingPipelineStateVK* pPipelineState = DBG_NEW RayTracingPipelineStateVK(this);
		if (!pPipelineState->Init(pDesc))
		{
			pPipelineState->Release();
			return nullptr;
		}
		else
		{
			return pPipelineState;
		}
	}

	IAccelerationStructure* GraphicsDeviceVK::CreateAccelerationStructure(const AccelerationStructureDesc* pDesc, IDeviceAllocator* pAllocator) const
	{
        VALIDATE(pDesc != nullptr);
        
		//TODO: Query this in some other way
		if (this->vkCreateAccelerationStructureKHR == nullptr)
		{
			return nullptr;
		}

		AccelerationStructureVK* pAccelerationStructure = DBG_NEW AccelerationStructureVK(this);
		if (!pAccelerationStructure->Init(pDesc, pAllocator))
		{
			pAccelerationStructure->Release();
			return nullptr;
		}
		else
		{
			return pAccelerationStructure;
		}
	}

	ICommandList* GraphicsDeviceVK::CreateCommandList(ICommandAllocator* pAllocator, const CommandListDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
        CommandListVK* pCommandListVK = DBG_NEW CommandListVK(this);
        if (!pCommandListVK->Init(pAllocator, pDesc))
        {
            pCommandListVK->Release();
            return nullptr;
        }
		else
		{
			return pCommandListVK;
		}
	}

	ICommandAllocator* GraphicsDeviceVK::CreateCommandAllocator(const char* pName, ECommandQueueType queueType) const
	{
		CommandAllocatorVK* pCommandAllocator = DBG_NEW CommandAllocatorVK(this);
		if (!pCommandAllocator->Init(pName, queueType))
		{
			pCommandAllocator->Release();
			return nullptr;
		}
		else
		{
			return pCommandAllocator;
		}
	}

	ICommandQueue* GraphicsDeviceVK::CreateCommandQueue(const char* pName, ECommandQueueType queueType) const
	{
        uint32  index               = 0;
		int32   queueFamilyIndex    = 0;
		if (queueType == ECommandQueueType::COMMAND_QUEUE_GRAPHICS)
		{
			queueFamilyIndex    = m_DeviceQueueFamilyIndices.GraphicsFamily;
            index               = m_NextGraphicsQueue++;
		}
		else if (queueType == ECommandQueueType::COMMAND_QUEUE_COMPUTE)
		{
			queueFamilyIndex    = m_DeviceQueueFamilyIndices.ComputeFamily;
            index               = m_NextComputeQueue++;
		}
		else if (queueType == ECommandQueueType::COMMAND_QUEUE_COPY)
		{
			queueFamilyIndex    = m_DeviceQueueFamilyIndices.TransferFamily;
            index               = m_NextTransferQueue++;
		}
		else
		{
			return nullptr;
		}
        
        ASSERT(queueFamilyIndex < int32(m_QueueFamilyProperties.size()));
        ASSERT(index            < m_QueueFamilyProperties[queueFamilyIndex].queueCount);

		CommandQueueVK* pQueue = DBG_NEW CommandQueueVK(this);
		if (!pQueue->Init(pName, queueFamilyIndex, index))
		{
			pQueue->Release();
			return nullptr;
		}
		else
		{
			return pQueue;
		}
	}

	IFence* GraphicsDeviceVK::CreateFence(const FenceDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
		if (IsDeviceExtensionEnabled(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
		{
			FenceVK* pFenceVk = DBG_NEW FenceVK(this);
			if (!pFenceVk->Init(pDesc))
			{
				pFenceVk->Release();
				return nullptr;
			}
			else
			{
				return pFenceVk;
			}
		}
		else
		{
			FenceLegacyVK* pFenceLegacyVk = DBG_NEW FenceLegacyVK(this);
			if (!pFenceLegacyVk->Init(pDesc))
			{
				pFenceLegacyVk->Release();
				return nullptr;
			}
			else
			{
				return pFenceLegacyVk;
			}
		}
	}

    IDeviceAllocator* GraphicsDeviceVK::CreateDeviceAllocator(const DeviceAllocatorDesc* pDesc) const
    {
        VALIDATE(pDesc != nullptr);
        
        DeviceAllocatorVK* pAllocator = DBG_NEW DeviceAllocatorVK(this);
        if (!pAllocator->Init(pDesc))
        {
            pAllocator->Release();
            return nullptr;
        }
        else
        {
            return pAllocator;
        }
    }

	IBuffer* GraphicsDeviceVK::CreateBuffer(const BufferDesc* pDesc, IDeviceAllocator* pAllocator) const
	{
        VALIDATE(pDesc != nullptr);
        
		BufferVK* pBuffer = DBG_NEW BufferVK(this);
		if (!pBuffer->Init(pDesc, pAllocator))
		{
            pBuffer->Release();
			return nullptr;
		}
		else
		{
			return pBuffer;
		}
	}

	ITexture* GraphicsDeviceVK::CreateTexture(const TextureDesc* pDesc, IDeviceAllocator* pAllocator) const
	{
        VALIDATE(pDesc != nullptr);
        
		TextureVK* pTexture = DBG_NEW TextureVK(this);
		if (!pTexture->Init(pDesc, pAllocator))
		{
            pTexture->Release();
			return nullptr;
		}
		else
		{
			return pTexture;
		}
	}

	ISampler* GraphicsDeviceVK::CreateSampler(const SamplerDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
		SamplerVK* pSampler = DBG_NEW SamplerVK(this);
		if (!pSampler->Init(pDesc))
		{
			pSampler->Release();
			return nullptr;
		}
		else
		{
			return pSampler;
		}
	}

	ITextureView* GraphicsDeviceVK::CreateTextureView(const TextureViewDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
        TextureViewVK* pTextureView = DBG_NEW TextureViewVK(this);
        if (!pTextureView->Init(pDesc))
        {
            pTextureView->Release();
            return nullptr;
        }
		else
		{
			return pTextureView;
		}
	}

	IShader* GraphicsDeviceVK::CreateShader(const ShaderDesc* pDesc) const
	{
        VALIDATE(pDesc != nullptr);
        
		ShaderVK* pShader = DBG_NEW ShaderVK(this);
		if (!pShader->Init(pDesc))
		{
			pShader->Release();
			return nullptr;
		}
		else
		{
			return pShader;
		}
	}

    ISwapChain* GraphicsDeviceVK::CreateSwapChain(const Window* pWindow, ICommandQueue* pCommandQueue, const SwapChainDesc* pDesc) const
    {
        VALIDATE(pDesc          != nullptr);
        VALIDATE(pWindow        != nullptr);
        VALIDATE(pCommandQueue  != nullptr);
        
        SwapChainVK* pSwapChain = DBG_NEW SwapChainVK(this);
        if (!pSwapChain->Init(pWindow, pCommandQueue, pDesc))
        {
            pSwapChain->Release();
            return nullptr;
        }
		else
		{
			return pSwapChain;
		}
    }

    /*
     * Copy Descriptor sets
     */

    void GraphicsDeviceVK::CopyDescriptorSet(const IDescriptorSet* pSrc, IDescriptorSet* pDst) const
    {
        DescriptorSetVK*        pDstVk            = reinterpret_cast<DescriptorSetVK*>(pDst);
        const DescriptorSetVK*    pSrcVk            = reinterpret_cast<const DescriptorSetVK*>(pSrc);
        uint32                    bindingCount    = pSrcVk->GetDescriptorBindingDescCount();

        VkCopyDescriptorSet copyDescriptorSet = {};
        copyDescriptorSet.sType                = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
        copyDescriptorSet.pNext                = nullptr;
        copyDescriptorSet.dstSet            = pDstVk->GetDescriptorSet();
        copyDescriptorSet.srcSet            = pSrcVk->GetDescriptorSet();
        copyDescriptorSet.srcArrayElement    = 0;
        copyDescriptorSet.dstArrayElement    = 0;

        std::vector<VkCopyDescriptorSet> descriptorSetCopies;
        descriptorSetCopies.reserve(bindingCount);
        for (uint32 i = 0; i < bindingCount; i++)
        {
            DescriptorBindingDesc binding = pSrcVk->GetDescriptorBindingDesc(i);

            copyDescriptorSet.descriptorCount    = binding.DescriptorCount;
            copyDescriptorSet.srcBinding        = binding.Binding;
            copyDescriptorSet.dstBinding        = copyDescriptorSet.srcBinding;
            
            descriptorSetCopies.push_back(copyDescriptorSet);
        }

        vkUpdateDescriptorSets(Device, 0, nullptr, uint32(descriptorSetCopies.size()), descriptorSetCopies.data());
    }

    void GraphicsDeviceVK::CopyDescriptorSet(const IDescriptorSet* pSrc, IDescriptorSet* pDst, const CopyDescriptorBindingDesc* pCopyBindings, uint32 copyBindingCount) const
    {
        DescriptorSetVK*        pDstVk = reinterpret_cast<DescriptorSetVK*>(pDst);
        const DescriptorSetVK*    pSrcVk = reinterpret_cast<const DescriptorSetVK*>(pSrc);

        VkCopyDescriptorSet copyDescriptorSet = {};
        copyDescriptorSet.sType                = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
        copyDescriptorSet.pNext                = nullptr;
        copyDescriptorSet.dstSet            = pDstVk->GetDescriptorSet();
        copyDescriptorSet.srcSet            = pSrcVk->GetDescriptorSet();
        copyDescriptorSet.srcArrayElement    = 0;
        copyDescriptorSet.dstArrayElement    = 0;

        std::vector<VkCopyDescriptorSet> descriptorSetCopies;
        descriptorSetCopies.reserve(copyBindingCount);
        for (uint32 i = 0; i < copyBindingCount; i++)
        {
            copyDescriptorSet.descriptorCount    = pCopyBindings[i].DescriptorCount;
            copyDescriptorSet.dstBinding        = pCopyBindings[i].DstBinding;
            copyDescriptorSet.srcBinding        = pCopyBindings[i].SrcBinding;

            descriptorSetCopies.push_back(copyDescriptorSet);
        }
        
        vkUpdateDescriptorSets(Device, 0, nullptr, uint32(descriptorSetCopies.size()), descriptorSetCopies.data());
    }

    /*
     * Helpers
     */

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

	VkPhysicalDeviceProperties GraphicsDeviceVK::GetPhysicalDeviceProperties() const
	{
		VkPhysicalDeviceProperties physicalDeviceProperties = {};
		vkGetPhysicalDeviceProperties(PhysicalDevice, &physicalDeviceProperties);
		
		return physicalDeviceProperties;
	}

    /*
     * Init GraphicsDevice
     */

	bool GraphicsDeviceVK::InitInstance(const GraphicsDeviceDesc* pDesc)
	{
		if (pDesc->Debug)
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

		LOG_MESSAGE("[GraphicsDeviceVK]: Requsted API Version: %u.%u.%u (%u)", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion), appInfo.apiVersion);

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo         = &appInfo;
		instanceCreateInfo.enabledExtensionCount    = (uint32_t)m_EnabledInstanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames  = m_EnabledInstanceExtensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		if (pDesc->Debug)
		{
			PopulateDebugMessengerCreateInfo(debugCreateInfo);

			instanceCreateInfo.enabledLayerCount    = (uint32_t)m_EnabledValidationLayers.size();
			instanceCreateInfo.ppEnabledLayerNames  = m_EnabledValidationLayers.data();
			instanceCreateInfo.pNext                = &debugCreateInfo;
		}
		else
		{
			instanceCreateInfo.enabledLayerCount    = 0;
			instanceCreateInfo.pNext                = nullptr;
		}

		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &Instance);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[GraphicsDeviceVK]: Failed to create Vulkan Instance!");
			return false;
		}

		RegisterInstanceExtensionData();

		if (pDesc->Debug)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			PopulateDebugMessengerCreateInfo(createInfo);

			result = vkCreateDebugUtilsMessengerEXT(Instance, &createInfo, nullptr, &m_DebugMessenger);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[GraphicsDeviceVK]: Failed to set up Debug Messenger!");
				return false;
			}
		}

		return true;
	}

	bool GraphicsDeviceVK::InitDevice(const GraphicsDeviceDesc* pDesc)
	{
		if (!InitPhysicalDevice())
		{
			LOG_ERROR("[GraphicsDeviceVK]: Could not initialize Physical Device!");
			return false;
		}

		if (!InitLogicalDevice(pDesc))
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

        //Store the properties of each queuefamily
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, nullptr);

        m_QueueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());
        
		// Save device's limits
		VkPhysicalDeviceProperties deviceProperties = GetPhysicalDeviceProperties();
		m_DeviceLimits = deviceProperties.limits;

		LOG_MESSAGE("[GraphicsDeviceVK]: Chosen device: %s", deviceProperties.deviceName);
		LOG_MESSAGE("[GraphicsDeviceVK]: API Version: %u.%u.%u (%u)", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion), deviceProperties.apiVersion);

		return true;
	}

	bool GraphicsDeviceVK::InitLogicalDevice(const GraphicsDeviceDesc* pDesc)
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
        deviceFeatures11.pNext  = nullptr;
		deviceFeatures11.pNext	= &deviceFeatures12;

		VkPhysicalDeviceFeatures desiredDeviceFeatures = {};
		desiredDeviceFeatures.fillModeNonSolid					= true;
		desiredDeviceFeatures.vertexPipelineStoresAndAtomics	= true;
		desiredDeviceFeatures.fragmentStoresAndAtomics			= true;
		desiredDeviceFeatures.multiDrawIndirect					= true;

		VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
		deviceFeatures2.sType	 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext	 = &deviceFeatures11;
		deviceFeatures2.features = desiredDeviceFeatures;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext					= &deviceFeatures2;
		createInfo.flags					= 0;
		createInfo.queueCreateInfoCount		= (uint32)queueCreateInfos.size();
		createInfo.pQueueCreateInfos		= queueCreateInfos.data();
		createInfo.enabledExtensionCount    = (uint32)m_EnabledDeviceExtensions.size();
		createInfo.ppEnabledExtensionNames  = m_EnabledDeviceExtensions.data();

		if (pDesc->Debug)
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
			LOG_VULKAN_ERROR(result, "[GraphicsDeviceVK]: Failed to create logical device!");
			return false;
		}
        else
        {
            D_LOG_MESSAGE("[GraphicsDeviceVK]: Created Device");
            return true;
        }
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

	bool GraphicsDeviceVK::IsInstanceExtensionEnabled(const char* pExtensionName) const
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

	bool GraphicsDeviceVK::IsDeviceExtensionEnabled(const char* pExtensionName) const
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
		//RayTracing
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

		//Timeline semaphores
		if (IsDeviceExtensionEnabled(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
		{
			GET_DEVICE_PROC_ADDR(Device, vkWaitSemaphores);
			GET_DEVICE_PROC_ADDR(Device, vkSignalSemaphore);
			GET_DEVICE_PROC_ADDR(Device, vkGetSemaphoreCounterValue);
		}

        //Buffer Address
        if (IsDeviceExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
        {
            GET_DEVICE_PROC_ADDR(Device, vkGetBufferDeviceAddress);
        }

	}

	VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsDeviceVK::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		UNREFERENCED_VARIABLE(messageType);
		UNREFERENCED_VARIABLE(pCallbackData);
		UNREFERENCED_VARIABLE(pUserData);

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			LOG_MESSAGE("[Validation Layer]: %s\n", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			LOG_MESSAGE("[Validation Layer]: %s\n", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LOG_WARNING("[Validation Layer]: %s\n", pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LOG_ERROR("[Validation Layer]: %s\n", pCallbackData->pMessage);
		}

		return VK_FALSE;
	}
}
