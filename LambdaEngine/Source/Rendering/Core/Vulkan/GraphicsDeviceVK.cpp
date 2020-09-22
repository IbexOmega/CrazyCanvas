#include <set>
#include <map>

#include "Log/Log.h"

#include "Application/API/PlatformConsole.h"

#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/FenceVK.h"
#include "Rendering/Core/Vulkan/FenceTimelineVK.h"
#include "Rendering/Core/Vulkan/SamplerVK.h"
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
#include "Rendering/Core/Vulkan/QueryHeapVK.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/SBTVK.h"

#define ENABLE_IF_SUPPORTED(feature) feature = feature && true;

namespace LambdaEngine
{
	/*
	 * ValidationLayers and Extensions
	 */
	constexpr ValidationLayer REQUIRED_VALIDATION_LAYERS[]
	{
		ValidationLayer("REQ_V_L_BASE"),
		ValidationLayer("VK_LAYER_KHRONOS_validation"),
		//ValidationLayer("VK_LAYER_RENDERDOC_Capture"),
	};

	constexpr ValidationLayer OPTIONAL_VALIDATION_LAYERS[]
	{
		ValidationLayer("OPT_V_L_BASE"),
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
	};

	constexpr Extension REQUIRED_DEVICE_EXTENSIONS[]
	{
		Extension("REQ_D_E_BASE"),
		Extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME),
	};

	constexpr Extension OPTIONAL_DEVICE_EXTENSIONS[]
	{
		Extension("OPT_D_E_BASE"),
		Extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME),
		Extension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME),
		Extension(VK_KHR_RAY_TRACING_EXTENSION_NAME),
		Extension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME),
		Extension(VK_NV_MESH_SHADER_EXTENSION_NAME),
		Extension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME),
		//Extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME)
	};

	/*
	 * GraphicsDeviceVK
	 */
	GraphicsDeviceVK::GraphicsDeviceVK()
		: GraphicsDevice()
		, RayTracingProperties()
		, m_DeviceQueueFamilyIndices()
		, m_DeviceLimits()
		, m_QueueFamilyProperties()
		, m_EnabledValidationLayers()
		, m_EnabledInstanceExtensions()
		, m_EnabledDeviceExtensions()
	{
	}

	GraphicsDeviceVK::~GraphicsDeviceVK()
	{
		SAFEDELETE(m_pFrameBufferCache);

		SAFERELEASE(m_pAccelerationStructureAllocator);
		SAFERELEASE(m_pTextureAllocator);
		SAFERELEASE(m_pVBAllocator);
		SAFERELEASE(m_pIBAllocator);
		SAFERELEASE(m_pCBAllocator);
		SAFERELEASE(m_pUAAllocator);
		SAFERELEASE(m_pBufferAllocator);

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

		if (!InitAllocators())
		{
			LOG_ERROR("[GraphicsDeviceVK]: Could not create deviceallocators!");
			return false;
		}
		else
		{
			LOG_MESSAGE("[GraphicsDeviceVK]: Created vulkan allocators!");
		}

		return true;
	}

	bool GraphicsDeviceVK::AllocateBufferMemory(AllocationVK* pAllocation, FBufferFlags bufferFlags, uint64 sizeInBytes, uint64 alignment, uint32 memoryIndex) const
	{
		VkDeviceSize alignedSize = AlignUp(sizeInBytes, alignment);
		if (alignedSize < LARGE_BUFFER_ALLOCATION_SIZE)
		{
			// Mask out buffer flags that does not describe a type
			const FBufferFlags discardedFlagsMask = ~(FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_COPY_SRC);
			const FBufferFlags flags = bufferFlags & discardedFlagsMask;

			DeviceAllocatorVK* pAllocator = nullptr;
			if (flags == FBufferFlag::BUFFER_FLAG_VERTEX_BUFFER)
			{
				pAllocator = m_pVBAllocator;
			}
			else if (flags == FBufferFlag::BUFFER_FLAG_INDEX_BUFFER)
			{
				pAllocator = m_pIBAllocator;
			}
			else if (flags == FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER)
			{
				pAllocator = m_pUAAllocator;
			}
			else if (flags == FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER)
			{
				pAllocator = m_pCBAllocator;
			}
			else
			{
				pAllocator = m_pBufferAllocator;
			}

			VALIDATE(pAllocator != nullptr);
			return pAllocator->Allocate(pAllocation, sizeInBytes, alignment, memoryIndex);
		}
		else
		{
			pAllocation->Offset = 0;
			pAllocation->pAllocator = nullptr;
			pAllocation->pBlock = nullptr;
			return (AllocateMemory(&pAllocation->Memory, sizeInBytes, memoryIndex) == VK_SUCCESS);
		}
	}

	bool GraphicsDeviceVK::AllocateAccelerationStructureMemory(AllocationVK* pAllocation, uint64 sizeInBytes, uint64 alignment, uint32 memoryIndex) const
	{
		VALIDATE(m_pAccelerationStructureAllocator != nullptr);
		VALIDATE(pAllocation != nullptr);

		VkDeviceSize alignedSize = AlignUp(sizeInBytes, alignment);
		if (alignedSize >= LARGE_ACCELERATION_STRUCTURE_ALLOCATION_SIZE)
		{
			pAllocation->Offset = 0;
			pAllocation->pAllocator = nullptr;
			pAllocation->pBlock = nullptr;
			return (AllocateMemory(&pAllocation->Memory, sizeInBytes, memoryIndex) == VK_SUCCESS);
		}
		else
		{
			return m_pAccelerationStructureAllocator->Allocate(pAllocation, sizeInBytes, alignment, memoryIndex);
		}
	}

	bool GraphicsDeviceVK::AllocateTextureMemory(AllocationVK* pAllocation, uint64 sizeInBytes, uint64 alignment, uint32 memoryIndex) const
	{
		VALIDATE(m_pTextureAllocator != nullptr);
		VALIDATE(pAllocation != nullptr);
		
		VkDeviceSize alignedSize = AlignUp(sizeInBytes, alignment);
		if (alignedSize >= LARGE_TEXTURE_ALLOCATION_SIZE)
		{
			pAllocation->Offset = 0;
			pAllocation->pAllocator = nullptr;
			pAllocation->pBlock = nullptr;
			return (AllocateMemory(&pAllocation->Memory, sizeInBytes, memoryIndex) == VK_SUCCESS);
		}
		else
		{
			return m_pTextureAllocator->Allocate(pAllocation, sizeInBytes, alignment, memoryIndex);
		}
	}

	bool GraphicsDeviceVK::FreeMemory(AllocationVK* pAllocation) const
	{
		VALIDATE(pAllocation != nullptr);

		// If the memory block is nullptr assume that the allocation is dedicated
		if (pAllocation->pBlock != nullptr)
		{
			DeviceAllocatorVK* pAllocator = pAllocation->pAllocator;
			VALIDATE(pAllocator != nullptr);
			return pAllocator->Free(pAllocation);
		}
		else
		{
			FreeMemory(pAllocation->Memory);
			return true;
		}
	}

	void* GraphicsDeviceVK::MapBufferMemory(AllocationVK* pAllocation) const
	{
		VALIDATE(pAllocation != nullptr);

		if (pAllocation->pBlock != nullptr)
		{
			DeviceAllocatorVK* pAllocator = pAllocation->pAllocator;
			
			VALIDATE(pAllocator != nullptr);
			VALIDATE(pAllocator != m_pTextureAllocator);

			return pAllocator->Map(pAllocation);
		}
		else
		{
			void* pHostMemory = nullptr;
			vkMapMemory(Device, pAllocation->Memory, 0, VK_WHOLE_SIZE, 0, &pHostMemory);
			return pHostMemory;
		}
	}

	void GraphicsDeviceVK::UnmapBufferMemory(AllocationVK* pAllocation) const
	{
		VALIDATE(pAllocation != nullptr);

		if (pAllocation->pBlock != nullptr)
		{
			DeviceAllocatorVK* pAllocator = pAllocation->pAllocator;
			
			VALIDATE(pAllocator != nullptr);
			VALIDATE(pAllocator != m_pTextureAllocator);

			pAllocator->Unmap(pAllocation);
		}
		else
		{
			vkUnmapMemory(Device, pAllocation->Memory);
		}
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
		if (result == VK_SUCCESS)
		{
			m_UsedAllocations++;
			D_LOG_INFO("[GraphicsDeviceVK]: Allocated %u bytes. Allocations %u/%u", sizeInBytes, m_UsedAllocations, m_DeviceLimits.maxMemoryAllocationCount);
		}
		else
		{
			LOG_VULKAN_ERROR(result, "[GraphicsDeviceVK]: Failed to allocate memory");
		}

		return result;
	}

	void GraphicsDeviceVK::FreeMemory(VkDeviceMemory deviceMemory) const
	{
		VALIDATE(deviceMemory != VK_NULL_HANDLE);

		vkFreeMemory(Device, deviceMemory, nullptr);

		m_UsedAllocations--;
		D_LOG_INFO("[GraphicsDeviceVK]: Freed memoryblock. Allocations %u/%u", m_UsedAllocations, m_DeviceLimits.maxMemoryAllocationCount);
	}

	void GraphicsDeviceVK::DestroyRenderPass(VkRenderPass* pRenderPass) const
	{
		VALIDATE(m_pFrameBufferCache != nullptr);

		if (*pRenderPass != VK_NULL_HANDLE)
		{
			m_pFrameBufferCache->DestroyRenderPass(*pRenderPass);

			vkDestroyRenderPass(Device, *pRenderPass, nullptr);
			*pRenderPass = VK_NULL_HANDLE;
		}
	}

	void GraphicsDeviceVK::DestroyImageView(VkImageView* pImageView) const
	{
		VALIDATE(m_pFrameBufferCache != nullptr);

		if (*pImageView != VK_NULL_HANDLE)
		{
			m_pFrameBufferCache->DestroyImageView(*pImageView);

			vkDestroyImageView(Device, *pImageView, nullptr);
			*pImageView = VK_NULL_HANDLE;
		}
	}

	VkFramebuffer GraphicsDeviceVK::GetFrameBuffer(const RenderPass* pRenderPass, const TextureView* const* ppRenderTargets, uint32 renderTargetCount, const TextureView* pDepthStencil, uint32 width, uint32 height) const
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

	void GraphicsDeviceVK::QueryDeviceFeatures(GraphicsDeviceFeatureDesc* pFeatures) const
	{
		memcpy(pFeatures, &m_DeviceFeatures, sizeof(m_DeviceFeatures));
	}

	void GraphicsDeviceVK::QueryDeviceMemoryStatistics(uint32* statCount, TArray<GraphicsDeviceMemoryStatistics>& pMemoryStat) const
	{
		// Queries each time function is called to get that _fresh_ information
		VkPhysicalDeviceMemoryBudgetPropertiesEXT memBudgetProp = {};
		memBudgetProp.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

		VkPhysicalDeviceMemoryProperties2 memProp2 = {};
		memProp2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		memProp2.pNext = (void*)(&memBudgetProp);

		vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice, &memProp2);

		if (*statCount == 0)
		{
			*statCount = memProp2.memoryProperties.memoryHeapCount;
		}
		else
		{
			for (uint32 i = 0; i < *statCount; i++)
			{
				pMemoryStat[i].TotalBytesReserved = memBudgetProp.heapBudget[i];
				pMemoryStat[i].TotalBytesAllocated = memBudgetProp.heapUsage[i];

				// Only set if name has not been set yet
				if (pMemoryStat[i].MemoryTypeName == "")
				{
					// Set memory type name
					for (uint32 typeIdx = 0; typeIdx < memProp2.memoryProperties.memoryTypeCount; typeIdx++)
					{
						if (memProp2.memoryProperties.memoryTypes[typeIdx].heapIndex == i)
						{
							VkMemoryPropertyFlags propFlag = memProp2.memoryProperties.memoryTypes[typeIdx].propertyFlags;
							if ((propFlag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && (propFlag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (propFlag & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
							{
								pMemoryStat[i].MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
								pMemoryStat[i].MemoryTypeName 	= "Special Memory"; // Memory that is directly mappable on the GPU
							}
							else if ((propFlag & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && (propFlag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
							{
								pMemoryStat[i].MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
								pMemoryStat[i].MemoryTypeName 	= "Shared GPU Memory"; // Memory that is mappable on the CPU
							}
							else if (propFlag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
							{
								pMemoryStat[i].MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
								pMemoryStat[i].MemoryTypeName 	= "Dedicated GPU Memory"; // Non-mappable memory on the GPU (VRAM)
							}
							else
								pMemoryStat[i].MemoryTypeName = "Memory heap does not match known memory types";
						}
					}
				}
			}
		}
	}

	QueryHeap* GraphicsDeviceVK::CreateQueryHeap(const QueryHeapDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		QueryHeapVK* pQueryHeap = DBG_NEW QueryHeapVK(this);
		if (!pQueryHeap->Init(pDesc))
		{
			pQueryHeap->Release();
			return nullptr;
		}
		else
		{
			return pQueryHeap;
		}
	}

	PipelineLayout* GraphicsDeviceVK::CreatePipelineLayout(const PipelineLayoutDesc* pDesc) const
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

	DescriptorHeap* GraphicsDeviceVK::CreateDescriptorHeap(const DescriptorHeapDesc* pDesc) const
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

	DescriptorSet* GraphicsDeviceVK::CreateDescriptorSet(const String& debugname, const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, DescriptorHeap* pDescriptorHeap) const
	{
		VALIDATE(pPipelineLayout != nullptr);
		VALIDATE(pDescriptorHeap != nullptr);

		DescriptorSetVK* pDescriptorSet = DBG_NEW DescriptorSetVK(this);
		if (!pDescriptorSet->Init(debugname, pPipelineLayout, descriptorLayoutIndex, pDescriptorHeap))
		{
			pDescriptorSet->Release();
			return nullptr;
		}
		else
		{
			return pDescriptorSet;
		}
	}

	RenderPass* GraphicsDeviceVK::CreateRenderPass(const RenderPassDesc* pDesc) const
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

	PipelineState* GraphicsDeviceVK::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc* pDesc) const
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

	PipelineState* GraphicsDeviceVK::CreateComputePipelineState(const ComputePipelineStateDesc* pDesc) const
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

	PipelineState* GraphicsDeviceVK::CreateRayTracingPipelineState(const RayTracingPipelineStateDesc* pDesc) const
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

	SBT* GraphicsDeviceVK::CreateSBT(CommandQueue* pCommandQueue, const SBTDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		SBTVK* pSBT = DBG_NEW SBTVK(this);
		if (!pSBT->Init(pCommandQueue, pDesc))
		{
			pSBT->Release();
			return nullptr;
		}
		else
		{
			return pSBT;
		}
	}

	AccelerationStructure* GraphicsDeviceVK::CreateAccelerationStructure(const AccelerationStructureDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		if (!m_DeviceFeatures.RayTracing)
		{
			return nullptr;
		}

		AccelerationStructureVK* pAccelerationStructure = DBG_NEW AccelerationStructureVK(this);
		if (!pAccelerationStructure->Init(pDesc))
		{
			pAccelerationStructure->Release();
			return nullptr;
		}
		else
		{
			return pAccelerationStructure;
		}
	}

	CommandList* GraphicsDeviceVK::CreateCommandList(CommandAllocator* pAllocator, const CommandListDesc* pDesc) const
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

	CommandAllocator* GraphicsDeviceVK::CreateCommandAllocator(const String& debugname, ECommandQueueType queueType) const
	{
		CommandAllocatorVK* pCommandAllocator = DBG_NEW CommandAllocatorVK(this);
		if (!pCommandAllocator->Init(debugname, queueType))
		{
			pCommandAllocator->Release();
			return nullptr;
		}
		else
		{
			return pCommandAllocator;
		}
	}

	CommandQueue* GraphicsDeviceVK::CreateCommandQueue(const String& debugname, ECommandQueueType queueType) const
	{
		uint32  index               = 0;
		int32   queueFamilyIndex    = 0;
		if (queueType == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
		{
			queueFamilyIndex    = m_DeviceQueueFamilyIndices.GraphicsFamily;
			index               = m_NextGraphicsQueue++;
		}
		else if (queueType == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
		{
			queueFamilyIndex    = m_DeviceQueueFamilyIndices.ComputeFamily;
			index               = m_NextComputeQueue++;
		}
		else if (queueType == ECommandQueueType::COMMAND_QUEUE_TYPE_COPY)
		{
			queueFamilyIndex    = m_DeviceQueueFamilyIndices.TransferFamily;
			index               = m_NextTransferQueue++;
		}
		else
		{
			return nullptr;
		}

		VALIDATE(queueFamilyIndex < int32(m_QueueFamilyProperties.GetSize()));
		VALIDATE(index            < m_QueueFamilyProperties[queueFamilyIndex].queueCount);

		CommandQueueVK* pQueue = DBG_NEW CommandQueueVK(this);
		if (!pQueue->Init(debugname, queueFamilyIndex, index))
		{
			pQueue->Release();
			return nullptr;
		}
		else
		{
			return pQueue;
		}
	}

	Fence* GraphicsDeviceVK::CreateFence(const FenceDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		if (UseTimelineFences())
		{
			FenceTimelineVK* pFenceTimelineVk = DBG_NEW FenceTimelineVK(this);
			if (!pFenceTimelineVk->Init(pDesc))
			{
				pFenceTimelineVk->Release();
				return nullptr;
			}
			else
			{
				return pFenceTimelineVk;
			}
		}
		else
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
	}

	Buffer* GraphicsDeviceVK::CreateBuffer(const BufferDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		BufferVK* pBuffer = DBG_NEW BufferVK(this);
		if (!pBuffer->Init(pDesc))
		{
			pBuffer->Release();
			return nullptr;
		}
		else
		{
			return pBuffer;
		}
	}

	Texture* GraphicsDeviceVK::CreateTexture(const TextureDesc* pDesc) const
	{
		VALIDATE(pDesc != nullptr);

		TextureVK* pTexture = DBG_NEW TextureVK(this);
		if (!pTexture->Init(pDesc))
		{
			pTexture->Release();
			return nullptr;
		}
		else
		{
			return pTexture;
		}
	}

	Sampler* GraphicsDeviceVK::CreateSampler(const SamplerDesc* pDesc) const
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

	TextureView* GraphicsDeviceVK::CreateTextureView(const TextureViewDesc* pDesc) const
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

	Shader* GraphicsDeviceVK::CreateShader(const ShaderDesc* pDesc) const
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

	SwapChain* GraphicsDeviceVK::CreateSwapChain(const SwapChainDesc* pDesc) const
	{
		VALIDATE(pDesc			!= nullptr);
		VALIDATE(pDesc->pWindow	!= nullptr);
		VALIDATE(pDesc->pQueue	!= nullptr);

		SwapChainVK* pSwapChain = DBG_NEW SwapChainVK(this);
		if (!pSwapChain->Init(pDesc))
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

	void GraphicsDeviceVK::CopyDescriptorSet(const DescriptorSet* pSrc, DescriptorSet* pDst) const
	{
		DescriptorSetVK*		pDstVk			= reinterpret_cast<DescriptorSetVK*>(pDst);
		const DescriptorSetVK*	pSrcVk			= reinterpret_cast<const DescriptorSetVK*>(pSrc);
		uint32					bindingCount	= pSrcVk->GetDescriptorBindingDescCount();

		VkCopyDescriptorSet copyDescriptorSet = {};
		copyDescriptorSet.sType				= VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
		copyDescriptorSet.pNext				= nullptr;
		copyDescriptorSet.dstSet			= pDstVk->GetDescriptorSet();
		copyDescriptorSet.srcSet			= pSrcVk->GetDescriptorSet();
		copyDescriptorSet.srcArrayElement	= 0;
		copyDescriptorSet.dstArrayElement	= 0;

		TArray<VkCopyDescriptorSet> descriptorSetCopies;
		descriptorSetCopies.Reserve(bindingCount);
		for (uint32 i = 0; i < bindingCount; i++)
		{
			DescriptorBindingDesc binding = pSrcVk->GetDescriptorBindingDesc(i);

			copyDescriptorSet.descriptorCount	= binding.DescriptorCount;
			copyDescriptorSet.srcBinding		= binding.Binding;
			copyDescriptorSet.dstBinding		= copyDescriptorSet.srcBinding;

			descriptorSetCopies.PushBack(copyDescriptorSet);
		}

		vkUpdateDescriptorSets(Device, 0, nullptr, uint32(descriptorSetCopies.GetSize()), descriptorSetCopies.GetData());
	}

	void GraphicsDeviceVK::CopyDescriptorSet(const DescriptorSet* pSrc, DescriptorSet* pDst, const CopyDescriptorBindingDesc* pCopyBindings, uint32 copyBindingCount) const
	{
		DescriptorSetVK*		pDstVk = reinterpret_cast<DescriptorSetVK*>(pDst);
		const DescriptorSetVK*	pSrcVk = reinterpret_cast<const DescriptorSetVK*>(pSrc);

		VkCopyDescriptorSet copyDescriptorSet = {};
		copyDescriptorSet.sType				= VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
		copyDescriptorSet.pNext				= nullptr;
		copyDescriptorSet.dstSet			= pDstVk->GetDescriptorSet();
		copyDescriptorSet.srcSet			= pSrcVk->GetDescriptorSet();
		copyDescriptorSet.srcArrayElement	= 0;
		copyDescriptorSet.dstArrayElement	= 0;

		TArray<VkCopyDescriptorSet> descriptorSetCopies;
		descriptorSetCopies.Reserve(copyBindingCount);
		for (uint32 i = 0; i < copyBindingCount; i++)
		{
			copyDescriptorSet.descriptorCount	= pCopyBindings[i].DescriptorCount;
			copyDescriptorSet.dstBinding		= pCopyBindings[i].DstBinding;
			copyDescriptorSet.srcBinding		= pCopyBindings[i].SrcBinding;

			descriptorSetCopies.PushBack(copyDescriptorSet);
		}

		vkUpdateDescriptorSets(Device, 0, nullptr, uint32(descriptorSetCopies.GetSize()), descriptorSetCopies.GetData());
	}

	/*
	 * Helpers
	 */

	void GraphicsDeviceVK::SetVulkanObjectName(const String& debugname, uint64 objectHandle, VkObjectType type) const
	{
		if (!debugname.empty())
		{
			if (vkSetDebugUtilsObjectNameEXT)
			{
				VkDebugUtilsObjectNameInfoEXT info = {};
				info.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
				info.pNext			= nullptr;
				info.objectType		= type;
				info.objectHandle	= objectHandle;
				info.pObjectName	= debugname.c_str();
				vkSetDebugUtilsObjectNameEXT(Device, &info);
			}
		}
	}

	uint32 GraphicsDeviceVK::GetQueueFamilyIndexFromQueueType(ECommandQueueType type) const
	{
		if (type == ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS)
		{
			return m_DeviceQueueFamilyIndices.GraphicsFamily;
		}
		else if (type == ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE)
		{
			return m_DeviceQueueFamilyIndices.ComputeFamily;
		}
		else if (type == ECommandQueueType::COMMAND_QUEUE_TYPE_COPY)
		{
			return m_DeviceQueueFamilyIndices.TransferFamily;
		}
		else if (type == ECommandQueueType::COMMAND_QUEUE_TYPE_NONE)
		{
			return VK_QUEUE_FAMILY_IGNORED;
		}
		else
		{
			return 0xffffffff;
		}
	}

	ECommandQueueType GraphicsDeviceVK::GetCommandQueueTypeFromQueueIndex(uint32 queueFamilyIndex) const
	{
		if (queueFamilyIndex == uint32(m_DeviceQueueFamilyIndices.GraphicsFamily))
		{
			return ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
		}
		else if (queueFamilyIndex == uint32(m_DeviceQueueFamilyIndices.ComputeFamily))
		{
			return ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE;
		}
		else if (queueFamilyIndex == uint32(m_DeviceQueueFamilyIndices.TransferFamily))
		{
			return ECommandQueueType::COMMAND_QUEUE_TYPE_COPY;
		}

		return ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN;
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

		// USE API VERSION 1.2 for now, maybe change to 1.0 later
		VkApplicationInfo appInfo = {};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pNext				= nullptr;
		appInfo.pApplicationName	= "Lambda Engine";
		appInfo.applicationVersion	= VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName			= "Lambda Engine";
		appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion			= VK_API_VERSION_1_2;

		LOG_MESSAGE("[GraphicsDeviceVK]: Requsted API Version: %u.%u.%u (%u)", VK_VERSION_MAJOR(appInfo.apiVersion), VK_VERSION_MINOR(appInfo.apiVersion), VK_VERSION_PATCH(appInfo.apiVersion), appInfo.apiVersion);

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType					= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.flags					= 0;
		instanceCreateInfo.pApplicationInfo			= &appInfo;
		instanceCreateInfo.enabledExtensionCount	= (uint32)m_EnabledInstanceExtensions.GetSize();
		instanceCreateInfo.ppEnabledExtensionNames	= m_EnabledInstanceExtensions.GetData();

		VkValidationFeaturesEXT validationFeatures	= {};
		validationFeatures.sType					= VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;

		VkValidationFeatureEnableEXT enabledValidationFeatures[] =
		{
			VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
			VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
		};

		if (pDesc->Debug)
		{
			validationFeatures.pEnabledValidationFeatures		= enabledValidationFeatures;
			validationFeatures.enabledValidationFeatureCount	= ARR_SIZE(enabledValidationFeatures);
		}

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		const char* pKhronosValidationLayerName = "VK_LAYER_KHRONOS_validation";

		if (pDesc->Debug)
		{
			PopulateDebugMessengerCreateInfo(debugCreateInfo);

			debugCreateInfo.pNext = &validationFeatures;

			instanceCreateInfo.enabledLayerCount	= 1;
			instanceCreateInfo.ppEnabledLayerNames	= &pKhronosValidationLayerName;
			instanceCreateInfo.pNext				= &debugCreateInfo;
		}
		else
		{
			instanceCreateInfo.enabledLayerCount	= 0;
			instanceCreateInfo.ppEnabledLayerNames	= nullptr;
			instanceCreateInfo.pNext				= nullptr;
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

			createInfo.pNext = &validationFeatures;

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

		// Set up device features
		m_DeviceFeatures.MeshShaders		= IsDeviceExtensionEnabled(VK_NV_MESH_SHADER_EXTENSION_NAME);
		m_DeviceFeatures.RayTracing			= IsDeviceExtensionEnabled(VK_KHR_RAY_TRACING_EXTENSION_NAME);
		m_DeviceFeatures.GeometryShaders	= m_DeviceFeaturesVk.geometryShader;
		m_DeviceFeatures.TimestampPeriod	= m_DeviceLimits.timestampPeriod;
		memcpy(&m_DeviceFeatures.MaxComputeWorkGroupSize, m_DeviceLimits.maxComputeWorkGroupSize, sizeof(uint32) * 3);

		RegisterDeviceExtensionData();
		return true;
	}

	bool GraphicsDeviceVK::InitPhysicalDevice()
	{
		uint32 deviceCount = 0;
		vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			LOG_ERROR("[GraphicsDeviceVK]: Presentation is not supported by the selected physicaldevice");
			return false;
		}

		TArray<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(Instance, &deviceCount, physicalDevices.GetData());

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

		// Store the properties of each queuefamily
		uint32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, nullptr);

		m_QueueFamilyProperties.Resize(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.GetData());

		// Save device's limits
		VkPhysicalDeviceProperties deviceProperties = GetPhysicalDeviceProperties();
		m_DeviceLimits = deviceProperties.limits;

		vkGetPhysicalDeviceFeatures(PhysicalDevice, &m_DeviceFeaturesVk);

		LOG_MESSAGE("[GraphicsDeviceVK]: Chosen device: %s", deviceProperties.deviceName);
		LOG_MESSAGE("[GraphicsDeviceVK]: API Version: %u.%u.%u (%u)", VK_VERSION_MAJOR(deviceProperties.apiVersion), VK_VERSION_MINOR(deviceProperties.apiVersion), VK_VERSION_PATCH(deviceProperties.apiVersion), deviceProperties.apiVersion);

		return true;
	}

	bool GraphicsDeviceVK::InitLogicalDevice(const GraphicsDeviceDesc* pDesc)
	{
		TArray<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int32> uniqueQueueFamilies =
		{
			m_DeviceQueueFamilyIndices.GraphicsFamily,
			m_DeviceQueueFamilyIndices.ComputeFamily,
			m_DeviceQueueFamilyIndices.TransferFamily
		};

		float queuePriority = 1.0f;
		for (int32 queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex    = uint32(queueFamily);
			queueCreateInfo.queueCount          = 1;
			queueCreateInfo.pQueuePriorities    = &queuePriority;
			queueCreateInfos.PushBack(queueCreateInfo);
		}

		VkPhysicalDeviceRayTracingFeaturesKHR	supportedRayTracingFeatures		= {};
		VkPhysicalDeviceVulkan12Features		supportedDeviceFeatures12		= {};
		VkPhysicalDeviceVulkan11Features		supportedDeviceFeatures11		= {};
		VkPhysicalDeviceFeatures				supportedDeviceFeatures10;

		{
			VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
			supportedRayTracingFeatures.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;

			supportedDeviceFeatures12.sType			= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			supportedDeviceFeatures12.pNext			= &supportedRayTracingFeatures;

			supportedDeviceFeatures11.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
			supportedDeviceFeatures11.pNext		= &supportedDeviceFeatures12;

			deviceFeatures2.sType					= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			deviceFeatures2.pNext					= &supportedDeviceFeatures11;

			vkGetPhysicalDeviceFeatures2(PhysicalDevice, &deviceFeatures2);

			supportedDeviceFeatures10 = deviceFeatures2.features;
		}


		VkPhysicalDeviceRayTracingFeaturesKHR enabledRayTracingFeatures = {};
		enabledRayTracingFeatures.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;

		VkPhysicalDeviceVulkan12Features enabledDeviceFeatures12 = {};
		enabledDeviceFeatures12.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		enabledDeviceFeatures12.pNext		= &enabledRayTracingFeatures;

		VkPhysicalDeviceVulkan11Features enabledDeviceFeatures11 = {};
		enabledDeviceFeatures11.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		enabledDeviceFeatures11.pNext		= &enabledDeviceFeatures12;

		VkPhysicalDeviceFeatures enabledDeviceFeatures10 = {};

		enabledRayTracingFeatures.rayTracing						= supportedRayTracingFeatures.rayTracing;

		enabledDeviceFeatures12.bufferDeviceAddress					= supportedDeviceFeatures12.bufferDeviceAddress;
		enabledDeviceFeatures12.timelineSemaphore					= supportedDeviceFeatures12.timelineSemaphore;
		enabledDeviceFeatures12.descriptorIndexing					= supportedDeviceFeatures12.descriptorIndexing;

		enabledDeviceFeatures10.fillModeNonSolid					= supportedDeviceFeatures10.fillModeNonSolid;
		enabledDeviceFeatures10.vertexPipelineStoresAndAtomics		= supportedDeviceFeatures10.vertexPipelineStoresAndAtomics;
		enabledDeviceFeatures10.fragmentStoresAndAtomics			= supportedDeviceFeatures10.fragmentStoresAndAtomics;
		enabledDeviceFeatures10.multiDrawIndirect					= supportedDeviceFeatures10.multiDrawIndirect;
		enabledDeviceFeatures10.pipelineStatisticsQuery				= supportedDeviceFeatures10.pipelineStatisticsQuery;
		enabledDeviceFeatures10.imageCubeArray						= supportedDeviceFeatures10.imageCubeArray;

		VkPhysicalDeviceFeatures2 enabledDeviceFeatures2 = {};
		enabledDeviceFeatures2.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		enabledDeviceFeatures2.pNext		= &enabledDeviceFeatures11;
		enabledDeviceFeatures2.features		= enabledDeviceFeatures10;

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pNext					= &enabledDeviceFeatures2;
		createInfo.flags					= 0;
		createInfo.queueCreateInfoCount		= (uint32)queueCreateInfos.GetSize();
		createInfo.pQueueCreateInfos		= queueCreateInfos.GetData();
		createInfo.enabledExtensionCount    = (uint32)m_EnabledDeviceExtensions.GetSize();
		createInfo.ppEnabledExtensionNames  = m_EnabledDeviceExtensions.GetData();

		if (pDesc->Debug)
		{
			createInfo.enabledLayerCount    = (uint32)m_EnabledValidationLayers.GetSize();
			createInfo.ppEnabledLayerNames  = m_EnabledValidationLayers.GetData();
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

	bool GraphicsDeviceVK::InitAllocators()
	{
		m_pTextureAllocator = DBG_NEW DeviceAllocatorVK(this);
		if (!m_pTextureAllocator->Init("Device Texture Allocator", LARGE_TEXTURE_ALLOCATION_SIZE))
		{
			return false;
		}

		m_pBufferAllocator = DBG_NEW DeviceAllocatorVK(this);
		if (!m_pBufferAllocator->Init("Default Device Buffer Allocator", LARGE_BUFFER_ALLOCATION_SIZE))
		{
			return false;
		}

		m_pCBAllocator = DBG_NEW DeviceAllocatorVK(this);
		if (!m_pCBAllocator->Init("Device ConstantBuffer Allocator", LARGE_BUFFER_ALLOCATION_SIZE))
		{
			return false;
		}

		m_pUAAllocator = DBG_NEW DeviceAllocatorVK(this);
		if (!m_pUAAllocator->Init("Device UnorderedAccessBuffer Allocator", LARGE_BUFFER_ALLOCATION_SIZE))
		{
			return false;
		}

		m_pVBAllocator = DBG_NEW DeviceAllocatorVK(this);
		if (!m_pVBAllocator->Init("Device VertexBuffer Allocator", LARGE_BUFFER_ALLOCATION_SIZE))
		{
			return false;
		}

		m_pIBAllocator = DBG_NEW DeviceAllocatorVK(this);
		if (!m_pIBAllocator->Init("Device IndexBuffer Allocator", LARGE_BUFFER_ALLOCATION_SIZE))
		{
			return false;
		}

		m_pAccelerationStructureAllocator = DBG_NEW DeviceAllocatorVK(this);
		if (!m_pAccelerationStructureAllocator->Init("Device AccelerationStructure Allocator", LARGE_ACCELERATION_STRUCTURE_ALLOCATION_SIZE))
		{
			return false;
		}

		return true;
	}

	bool GraphicsDeviceVK::SetEnabledValidationLayers()
	{
		uint32 layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		TArray<VkLayerProperties> availableValidationLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableValidationLayers.GetData());

		TArray<ValidationLayer> requiredValidationLayers(REQUIRED_VALIDATION_LAYERS + 1, REQUIRED_VALIDATION_LAYERS + sizeof(REQUIRED_VALIDATION_LAYERS) / sizeof(ValidationLayer));
		TArray<ValidationLayer> optionalValidationLayers(OPTIONAL_VALIDATION_LAYERS + 1, OPTIONAL_VALIDATION_LAYERS + sizeof(OPTIONAL_VALIDATION_LAYERS) / sizeof(ValidationLayer));

		for (const VkLayerProperties& availableValidationLayerProperties : availableValidationLayers)
		{
			uint32 availableValidationLayerHash = HashString<const char*>(availableValidationLayerProperties.layerName);
			for (auto requiredValidationLayer = requiredValidationLayers.begin(); requiredValidationLayer != requiredValidationLayers.end(); requiredValidationLayer++)
			{
				if (availableValidationLayerHash == requiredValidationLayer->Hash)
				{
					m_EnabledValidationLayers.PushBack(requiredValidationLayer->Name);
					requiredValidationLayers.Erase(requiredValidationLayer);
					break;
				}
			}

			for (auto optionalValidationLayer = optionalValidationLayers.begin(); optionalValidationLayer != optionalValidationLayers.end(); optionalValidationLayer++)
			{
				if (availableValidationLayerHash == optionalValidationLayer->Hash)
				{
					m_EnabledValidationLayers.PushBack(optionalValidationLayer->Name);
					optionalValidationLayers.Erase(optionalValidationLayer);
					break;
				}
			}
		}

		if (requiredValidationLayers.GetSize() > 0)
		{
			for (const ValidationLayer& requiredValidationLayer : requiredValidationLayers)
			{
				LOG_ERROR("[GraphicsDeviceVK]: Required Validation Layer %s not supported", requiredValidationLayer.Name);
			}

			return false;
		}

		if (optionalValidationLayers.GetSize() > 0)
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
		uint32 extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		TArray<VkExtensionProperties> availableInstanceExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.GetData());

		TArray<Extension> requiredInstanceExtensions(REQUIRED_INSTANCE_EXTENSIONS + 1, REQUIRED_INSTANCE_EXTENSIONS + sizeof(REQUIRED_INSTANCE_EXTENSIONS) / sizeof(Extension));
		TArray<Extension> optionalInstanceExtensions(OPTIONAL_INSTANCE_EXTENSIONS + 1, OPTIONAL_INSTANCE_EXTENSIONS + sizeof(OPTIONAL_INSTANCE_EXTENSIONS) / sizeof(Extension));

		for (const VkExtensionProperties& extension : availableInstanceExtensions)
		{
			uint32 availableInstanceExtensionHash = HashString<const char*>(extension.extensionName);
			for (auto requiredInstanceExtension = requiredInstanceExtensions.begin(); requiredInstanceExtension != requiredInstanceExtensions.end(); requiredInstanceExtension++)
			{
				if (requiredInstanceExtension->Hash == availableInstanceExtensionHash)
				{
					m_EnabledInstanceExtensions.PushBack(requiredInstanceExtension->Name);
					requiredInstanceExtensions.Erase(requiredInstanceExtension);
					break;
				}
			}

			for (auto optionalInstanceExtension = optionalInstanceExtensions.begin(); optionalInstanceExtension != optionalInstanceExtensions.end(); optionalInstanceExtension++)
			{
				if (optionalInstanceExtension->Hash == availableInstanceExtensionHash)
				{
					m_EnabledInstanceExtensions.PushBack(optionalInstanceExtension->Name);
					optionalInstanceExtensions.Erase(optionalInstanceExtension);
					break;
				}
			}
		}

		if (requiredInstanceExtensions.GetSize() > 0)
		{
			for (const Extension& requiredInstanceExtension : requiredInstanceExtensions)
			{
				LOG_ERROR("[GraphicsDeviceVK]: Required Instance Extension %s not supported", requiredInstanceExtension.Name);
			}

			return false;
		}

		if (optionalInstanceExtensions.GetSize() > 0)
		{
			for (const Extension& optionalInstanceExtension : optionalInstanceExtensions)
			{
				LOG_WARNING("[GraphicsDeviceVK]: Optional Instance Extension %s not supported", optionalInstanceExtension.Name);
			}
		}

		return true;
	}

	int32 GraphicsDeviceVK::RatePhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		bool		requiredExtensionsSupported			= false;
		uint32	numOfOptionalExtensionsSupported	= 0;
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

		int32 score = 1 + numOfOptionalExtensionsSupported;
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}

		return score;
	}

	void GraphicsDeviceVK::CheckDeviceExtensionsSupport(VkPhysicalDevice physicalDevice, bool& requiredExtensionsSupported, uint32& numOfOptionalExtensionsSupported)
	{
		uint32 extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		TArray<VkExtensionProperties> availableDeviceExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableDeviceExtensions.GetData());

		TArray<Extension> requiredDeviceExtensions(REQUIRED_DEVICE_EXTENSIONS + 1, REQUIRED_DEVICE_EXTENSIONS + sizeof(REQUIRED_DEVICE_EXTENSIONS) / sizeof(Extension));
		TArray<Extension> optionalDeviceExtensions(OPTIONAL_DEVICE_EXTENSIONS + 1, OPTIONAL_DEVICE_EXTENSIONS + sizeof(OPTIONAL_DEVICE_EXTENSIONS) / sizeof(Extension));

		for (const VkExtensionProperties& extension : availableDeviceExtensions)
		{
			uint32 availableDeviceExtensionHash = HashString<const char*>(extension.extensionName);
			for (auto requiredDeviceExtension = requiredDeviceExtensions.begin(); requiredDeviceExtension != requiredDeviceExtensions.end(); requiredDeviceExtension++)
			{
				if (requiredDeviceExtension->Hash == availableDeviceExtensionHash)
				{
					requiredDeviceExtensions.Erase(requiredDeviceExtension);
					break;
				}
			}

			for (auto optionalDeviceExtension = optionalDeviceExtensions.begin(); optionalDeviceExtension != optionalDeviceExtensions.end(); optionalDeviceExtension++)
			{
				if (optionalDeviceExtension->Hash == availableDeviceExtensionHash)
				{
					optionalDeviceExtensions.Erase(optionalDeviceExtension);
					break;
				}
			}
		}

		requiredExtensionsSupported			= requiredDeviceExtensions.IsEmpty();
		numOfOptionalExtensionsSupported	= ARR_SIZE(OPTIONAL_DEVICE_EXTENSIONS) - (uint32)optionalDeviceExtensions.GetSize();
	}

	QueueFamilyIndices GraphicsDeviceVK::FindQueueFamilies(VkPhysicalDevice physicalDevice)
	{
		QueueFamilyIndices indices = {};

		uint32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		TArray<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.GetData());

		indices.GraphicsFamily  = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilies);
		indices.ComputeFamily   = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilies);
		indices.TransferFamily  = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilies);

		return indices;
	}

	void GraphicsDeviceVK::SetEnabledDeviceExtensions()
	{
		//We know all requried device extensions are supported
		for (uint32 i = 1; i < ARR_SIZE(REQUIRED_DEVICE_EXTENSIONS); i++)
		{
			m_EnabledDeviceExtensions.EmplaceBack(REQUIRED_DEVICE_EXTENSIONS[i].Name);
		}

		uint32 extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &extensionCount, nullptr);

		TArray<VkExtensionProperties> availableDeviceExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &extensionCount, availableDeviceExtensions.GetData());

		TArray<Extension> optionalDeviceExtensions(OPTIONAL_DEVICE_EXTENSIONS + 1, OPTIONAL_DEVICE_EXTENSIONS + sizeof(OPTIONAL_DEVICE_EXTENSIONS) / sizeof(Extension));
		for (const VkExtensionProperties& extension : availableDeviceExtensions)
		{
			uint32 availableDeviceExtensionHash = HashString<const char*>(extension.extensionName);
			for (auto optionalDeviceExtension = optionalDeviceExtensions.begin(); optionalDeviceExtension != optionalDeviceExtensions.end(); optionalDeviceExtension++)
			{
				if (optionalDeviceExtension->Hash == availableDeviceExtensionHash)
				{
					m_EnabledDeviceExtensions.EmplaceBack(optionalDeviceExtension->Name);
					optionalDeviceExtensions.Erase(optionalDeviceExtension);
					break;
				}
			}
		}

		if (optionalDeviceExtensions.GetSize() > 0)
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

	bool GraphicsDeviceVK::UseTimelineFences() const
	{
		return (vkWaitSemaphores != nullptr);
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
		// RayTracing
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
			GET_DEVICE_PROC_ADDR(Device, vkCopyAccelerationStructureToMemoryKHR);
			GET_DEVICE_PROC_ADDR(Device, vkCmdCopyAccelerationStructureToMemoryKHR);

			// Query Ray Tracing properties
			RayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
			RayTracingProperties.pNext = nullptr;

			VkPhysicalDeviceProperties2 deviceProps2 = {};
			deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProps2.pNext = &RayTracingProperties;

			vkGetPhysicalDeviceProperties2(PhysicalDevice, &deviceProps2);
		}

		//PushDescriptorSet
		if (IsDeviceExtensionEnabled(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME))
		{
			GET_DEVICE_PROC_ADDR(Device, vkCmdPushDescriptorSetKHR);
		}
		
		// Timeline semaphores
		if (IsDeviceExtensionEnabled(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))
		{
			GET_DEVICE_PROC_ADDR(Device, vkWaitSemaphores);
			GET_DEVICE_PROC_ADDR(Device, vkSignalSemaphore);
			GET_DEVICE_PROC_ADDR(Device, vkGetSemaphoreCounterValue);
		}

		// Mesh Shaders
		if (IsDeviceExtensionEnabled(VK_NV_MESH_SHADER_EXTENSION_NAME))
		{
			GET_DEVICE_PROC_ADDR(Device, vkCmdDrawMeshTasksNV);
			GET_DEVICE_PROC_ADDR(Device, vkCmdDrawMeshTasksIndirectNV);
			GET_DEVICE_PROC_ADDR(Device, vkCmdDrawMeshTasksIndirectCountNV);
		}
	}

	void GraphicsDeviceVK::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity	= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType		= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback	= DebugCallback;
		createInfo.pUserData		= nullptr;
	}

	uint32 GraphicsDeviceVK::GetQueueFamilyIndex(VkQueueFlagBits queueFlags, const TArray<VkQueueFamilyProperties>& queueFamilies)
	{
		if (queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32 i = 0; i < uint32(queueFamilies.GetSize()); i++)
			{
				if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					return i;
				}
			}
		}

		if (queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32 i = 0; i < uint32(queueFamilies.GetSize()); i++)
			{
				if ((queueFamilies[i].queueFlags & queueFlags) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					return i;
				}
			}
		}

		for (uint32 i = 0; i < uint32(queueFamilies.GetSize()); i++)
		{
			if (queueFamilies[i].queueFlags & queueFlags)
			{
				return i;
			}
		}

		return UINT32_MAX;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsDeviceVK::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		UNREFERENCED_VARIABLE(messageType);
		UNREFERENCED_VARIABLE(pCallbackData);
		UNREFERENCED_VARIABLE(pUserData);

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			LOG_MESSAGE("[Validation Layer]: %s", pCallbackData->pMessage);
			PlatformConsole::Print("\n");
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			LOG_MESSAGE("[Validation Layer]: %s", pCallbackData->pMessage);
			PlatformConsole::Print("\n");
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LOG_WARNING("[Validation Layer]: %s", pCallbackData->pMessage);
			PlatformConsole::Print("\n");
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LOG_ERROR("[Validation Layer]: %s", pCallbackData->pMessage);
			PlatformConsole::Print("\n");
		}

		return VK_FALSE;
	}
}
