#include "Log/Log.h"

#include <algorithm>

#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/ComputePipelineStateVK.h"
#include "Rendering/Core/Vulkan/GraphicsPipelineStateVK.h"
#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/RenderPassVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/DescriptorSetVK.h"
#include "Rendering/Core/Vulkan/QueryHeapVK.h"
#include "Rendering/Core/Vulkan/AccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#ifdef LAMBDA_DEBUG
	#define LAMBDA_VULKAN_CHECKS_ENABLED 1
#else
	#define LAMBDA_VULKAN_CHECKS_ENABLED 0
#endif

#if LAMBDA_VULKAN_CHECKS_ENABLED
	#define CHECK_GRAPHICS(pAllocator)	VALIDATE((pAllocator)->GetType() == LambdaEngine::ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
	#define CHECK_COMPUTE(pAllocator)	VALIDATE((pAllocator)->GetType() == LambdaEngine::ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
#else
	#define CHECK_GRAPHICS(pAllocator)
	#define CHECK_COMPUTE(pAllocator)
#endif

namespace LambdaEngine
{
	CommandListVK::CommandListVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_ImageBarriers(),
		m_Viewports(),
		m_ScissorRects(),
		m_VertexBuffers(),
		m_VertexBufferOffsets()
	{
	}

	CommandListVK::~CommandListVK()
	{
		// There has to be an allocator for the commandlist to be valid
		VALIDATE(m_Allocator != nullptr);
		m_Allocator->FreeCommandBuffer(m_CommandList);

		m_CommandList = VK_NULL_HANDLE;
	}

	bool CommandListVK::Init(CommandAllocator* pAllocator, const CommandListDesc* pDesc)
	{
		VkCommandBufferLevel level;
		if (pDesc->CommandListType == ECommandListType::COMMAND_LIST_TYPE_PRIMARY)
		{
			level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		}
		else
		{
			level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		}

		CommandAllocatorVK*	pVkCommandAllocator = (CommandAllocatorVK*)pAllocator;
		m_CommandList = pVkCommandAllocator->AllocateCommandBuffer(level);
		if (m_CommandList == VK_NULL_HANDLE)
		{
			return false;
		}
		else
		{
			m_Desc		= *pDesc;
			m_QueueType = pAllocator->GetType();
			m_Allocator = pVkCommandAllocator;
			SetName(pDesc->DebugName);
			
			return true;
		}
	}

	void CommandListVK::SetName(const String& debugname)
	{
			m_pDevice->SetVulkanObjectName(debugname, reinterpret_cast<uint64>(m_CommandList), VK_OBJECT_TYPE_COMMAND_BUFFER);
			m_Desc.DebugName = debugname;
	}

	bool CommandListVK::Begin(const SecondaryCommandListBeginDesc* pBeginDesc)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags	= 0;
		if (m_Desc.Flags & FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT)
		{
			beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		}
		
		VkCommandBufferInheritanceInfo inheritanceInfo = { };
		if (pBeginDesc)
		{
			const RenderPassVK*	pRenderPassVk = reinterpret_cast<const RenderPassVK*>(pBeginDesc->pRenderPass);

			VALIDATE(pRenderPassVk != nullptr);
			
			inheritanceInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritanceInfo.pNext					= nullptr;
			inheritanceInfo.framebuffer				= m_pDevice->GetFrameBuffer(pRenderPassVk, pBeginDesc->ppRenderTargets, pBeginDesc->RenderTargetCount, pBeginDesc->pDepthStencilView, pBeginDesc->Width, pBeginDesc->Height);
			inheritanceInfo.renderPass				= pRenderPassVk->GetRenderPass();
			inheritanceInfo.subpass					= pBeginDesc->SubPass;
			inheritanceInfo.occlusionQueryEnable	= VK_FALSE;
			inheritanceInfo.pipelineStatistics		= 0;
			inheritanceInfo.queryFlags				= 0;
			
			beginInfo.pInheritanceInfo = &inheritanceInfo;
		}
		else
		{
			beginInfo.pInheritanceInfo	= nullptr;
		}

		VkResult result = vkBeginCommandBuffer(m_CommandList, &beginInfo);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandListVK]: Begin CommandBuffer Failed");
			return false;
		}
		else
		{
			return true;
		}
	}

	bool CommandListVK::End()
	{
		VkResult result = vkEndCommandBuffer(m_CommandList); 
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandListVK]: End CommandBuffer Failed");
			return false;
		}
		else
		{
			return true;
		}
	}

	void CommandListVK::BeginRenderPass(const BeginRenderPassDesc* pBeginDesc)
	{
		VALIDATE(pBeginDesc != nullptr);

		const RenderPassVK*		pVkRenderPass	= reinterpret_cast<const RenderPassVK*>(pBeginDesc->pRenderPass);
		VkFramebuffer			vkFramebuffer	= m_pDevice->GetFrameBuffer(pBeginDesc->pRenderPass, pBeginDesc->ppRenderTargets, pBeginDesc->RenderTargetCount, pBeginDesc->pDepthStencil, pBeginDesc->Width, pBeginDesc->Height);

		VALIDATE(pVkRenderPass != nullptr);
		VALIDATE(vkFramebuffer != VK_NULL_HANDLE);

		for (uint32 i = 0; i < pBeginDesc->ClearColorCount; i++)
		{
			//TODO: Make a more safe version?
			const ClearColorDesc* colorDesc = &(pBeginDesc->pClearColors[i]);
			memcpy(m_ClearValues[i].color.float32, colorDesc, sizeof(ClearColorDesc));
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.pNext				= nullptr;
		renderPassInfo.renderPass			= pVkRenderPass->GetRenderPass();
		renderPassInfo.framebuffer			= vkFramebuffer;
		renderPassInfo.renderArea.offset	= { pBeginDesc->Offset.x, pBeginDesc->Offset.y };
		renderPassInfo.renderArea.extent	= { pBeginDesc->Width, pBeginDesc->Height };
		renderPassInfo.pClearValues			= m_ClearValues;
		renderPassInfo.clearValueCount		= pBeginDesc->ClearColorCount;

		VkSubpassContents subpassContent = VK_SUBPASS_CONTENTS_INLINE;
		if (pBeginDesc->Flags & FRenderPassBeginFlags::RENDER_PASS_BEGIN_FLAG_EXECUTE_SECONDARY)
		{
			subpassContent = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
		}

		vkCmdBeginRenderPass(m_CommandList, &renderPassInfo, subpassContent);
	}

	void CommandListVK::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandList);
	}

	void CommandListVK::BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* pBuildDesc)
	{
		VALIDATE(pBuildDesc != nullptr);

		VALIDATE(pBuildDesc->pAccelerationStructure != nullptr);
		VALIDATE(pBuildDesc->pInstanceBuffer		!= nullptr);
		AccelerationStructureVK* pAccelerationStructureVk	= reinterpret_cast<AccelerationStructureVK*>(pBuildDesc->pAccelerationStructure);
		BufferVK*		pScratchBufferVk					= pAccelerationStructureVk->GetScratchBuffer();
		const BufferVK* pInstanceBufferVk					= reinterpret_cast<const BufferVK*>(pBuildDesc->pInstanceBuffer);

		VkDeviceOrHostAddressConstKHR instancesDataAddressUnion = {};
		instancesDataAddressUnion.deviceAddress = pInstanceBufferVk->GetDeviceAdress();

		VkAccelerationStructureGeometryInstancesDataKHR instancesDataDesc = {};
		instancesDataDesc.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instancesDataDesc.arrayOfPointers	= VK_FALSE;
		instancesDataDesc.data				= instancesDataAddressUnion;

		VkAccelerationStructureGeometryDataKHR geometryDataUnion = {};
		geometryDataUnion.instances = instancesDataDesc;

		VkAccelerationStructureGeometryKHR geometryData = {};
		geometryData.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometryData.geometryType	= VK_GEOMETRY_TYPE_INSTANCES_KHR;
		geometryData.geometry		= geometryDataUnion;
		geometryData.flags			= 0;

		VkAccelerationStructureGeometryKHR* pGeometryData = &geometryData;

		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildInfo = {};
		accelerationStructureBuildInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildInfo.type						= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildInfo.geometryArrayOfPointers	= VK_FALSE;
		accelerationStructureBuildInfo.geometryCount			= 1;
		accelerationStructureBuildInfo.ppGeometries				= &pGeometryData;
		accelerationStructureBuildInfo.flags					= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		if (pBuildDesc->Flags & FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE)
		{
			accelerationStructureBuildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}

		accelerationStructureBuildInfo.scratchData.deviceAddress	= pScratchBufferVk->GetDeviceAdress();

		if (pBuildDesc->Update)
		{
			accelerationStructureBuildInfo.update					= VK_TRUE;
			accelerationStructureBuildInfo.srcAccelerationStructure	= pAccelerationStructureVk->GetAccelerationStructure();
			accelerationStructureBuildInfo.dstAccelerationStructure	= pAccelerationStructureVk->GetAccelerationStructure();
		}
		else
		{
			accelerationStructureBuildInfo.update					= VK_FALSE;
			accelerationStructureBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
			accelerationStructureBuildInfo.dstAccelerationStructure = pAccelerationStructureVk->GetAccelerationStructure();
		}

		VkAccelerationStructureBuildOffsetInfoKHR accelerationStructureOffsetInfo = {};
		accelerationStructureOffsetInfo.primitiveCount	= pBuildDesc->InstanceCount;
		accelerationStructureOffsetInfo.primitiveOffset = 0;

		VALIDATE(m_pDevice->vkCmdBuildAccelerationStructureKHR != nullptr);

		VkAccelerationStructureBuildOffsetInfoKHR* pAccelerationStructureOffsetInfo = &accelerationStructureOffsetInfo;
		m_pDevice->vkCmdBuildAccelerationStructureKHR(m_CommandList, 1, &accelerationStructureBuildInfo, &pAccelerationStructureOffsetInfo);
	}

	void CommandListVK::BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* pBuildDesc)
	{
		VALIDATE(pBuildDesc != nullptr);

		VALIDATE(pBuildDesc->pAccelerationStructure != nullptr);
		VALIDATE(pBuildDesc->pVertexBuffer			!= nullptr);
		VALIDATE(pBuildDesc->pIndexBuffer			!= nullptr);

		AccelerationStructureVK*	pAccelerationStructureVk	= reinterpret_cast<AccelerationStructureVK*>(pBuildDesc->pAccelerationStructure);
		BufferVK*					pScratchBufferVk			= pAccelerationStructureVk->GetScratchBuffer();
		const BufferVK*				pVertexBufferVk				= reinterpret_cast<const BufferVK*>(pBuildDesc->pVertexBuffer);
		const BufferVK*				pIndexBufferVk				= reinterpret_cast<const BufferVK*>(pBuildDesc->pIndexBuffer);

		VkDeviceOrHostAddressConstKHR vertexDataAddressUnion = {};
		vertexDataAddressUnion.deviceAddress = pVertexBufferVk->GetDeviceAdress();

		VkDeviceOrHostAddressConstKHR indexDataAddressUnion = {};
		indexDataAddressUnion.deviceAddress = pIndexBufferVk->GetDeviceAdress();

		VkAccelerationStructureGeometryTrianglesDataKHR geometryDataDesc = {};
		geometryDataDesc.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometryDataDesc.pNext			= nullptr;
		geometryDataDesc.vertexFormat	= VK_FORMAT_R32G32B32_SFLOAT;
		geometryDataDesc.vertexData		= vertexDataAddressUnion;
		geometryDataDesc.vertexStride	= pBuildDesc->VertexStride;
		geometryDataDesc.indexType		= VK_INDEX_TYPE_UINT32;
		geometryDataDesc.indexData		= indexDataAddressUnion;

		VkDeviceOrHostAddressConstKHR transformDataAddressUnion = {};
		if (pBuildDesc->pTransformBuffer != nullptr)
		{
			transformDataAddressUnion.deviceAddress = reinterpret_cast<const BufferVK*>(pBuildDesc->pTransformBuffer)->GetDeviceAdress();
			geometryDataDesc.transformData = transformDataAddressUnion;
		}
		else
		{
			geometryDataDesc.transformData = { 0 };
		}

		VkAccelerationStructureGeometryDataKHR geometryDataUnion = {};
		geometryDataUnion.triangles = geometryDataDesc;

		VkAccelerationStructureGeometryKHR geometryData = {};
		geometryData.sType			= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometryData.pNext			= nullptr;
		geometryData.geometryType	= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometryData.geometry		= geometryDataUnion;
		geometryData.flags			= VK_GEOMETRY_OPAQUE_BIT_KHR; //Cant be opaque if we want to utilize any-hit shaders

		VkDeviceOrHostAddressKHR scratchBufferAddressUnion = {};
		scratchBufferAddressUnion.deviceAddress = pScratchBufferVk->GetDeviceAdress();

		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildInfo = {};
		accelerationStructureBuildInfo.sType	= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildInfo.type		= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildInfo.flags	= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		if (pBuildDesc->Flags & FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE)
		{
			accelerationStructureBuildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}

		VkAccelerationStructureGeometryKHR* pGeometryData = &geometryData;
		accelerationStructureBuildInfo.geometryArrayOfPointers	= VK_FALSE;
		accelerationStructureBuildInfo.geometryCount			= 1;
		accelerationStructureBuildInfo.ppGeometries				= &pGeometryData;
		accelerationStructureBuildInfo.update					= VK_FALSE;
		accelerationStructureBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
		accelerationStructureBuildInfo.dstAccelerationStructure = pAccelerationStructureVk->GetAccelerationStructure();
		accelerationStructureBuildInfo.scratchData				= scratchBufferAddressUnion;

		VkAccelerationStructureBuildOffsetInfoKHR accelerationStructureOffsetInfo = {};
		accelerationStructureOffsetInfo.primitiveCount	= pBuildDesc->TriangleCount;
		accelerationStructureOffsetInfo.primitiveOffset = pBuildDesc->IndexBufferByteOffset;
		accelerationStructureOffsetInfo.firstVertex		= pBuildDesc->FirstVertexIndex;
		accelerationStructureOffsetInfo.transformOffset = pBuildDesc->TransformByteOffset;

		VALIDATE(m_pDevice->vkCmdBuildAccelerationStructureKHR != nullptr);

		VkAccelerationStructureBuildOffsetInfoKHR* pAccelerationStructureOffsetInfo = &accelerationStructureOffsetInfo;
		m_pDevice->vkCmdBuildAccelerationStructureKHR(m_CommandList, 1, &accelerationStructureBuildInfo, &pAccelerationStructureOffsetInfo);
	}

	void CommandListVK::CopyBuffer(const Buffer* pSrc, uint64 srcOffset, Buffer* pDst, uint64 dstOffset, uint64 sizeInBytes)
	{
		VALIDATE(pSrc != nullptr);
		VALIDATE(pDst != nullptr);

		const BufferVK* pVkSrc	= reinterpret_cast<const BufferVK*>(pSrc);
		BufferVK*		pVkDst	= reinterpret_cast<BufferVK*>(pDst);

		VkBufferCopy bufferCopy = {};
		bufferCopy.size			= sizeInBytes;
		bufferCopy.srcOffset	= srcOffset;
		bufferCopy.dstOffset	= dstOffset;

		vkCmdCopyBuffer(m_CommandList, pVkSrc->GetBuffer(), pVkDst->GetBuffer(), 1, &bufferCopy);
	}

	void CommandListVK::CopyTextureFromBuffer(const Buffer* pSrc, Texture* pDst, const CopyTextureFromBufferDesc& desc)
	{
		VALIDATE(pSrc != nullptr);
		VALIDATE(pDst != nullptr);

		const BufferVK* pVkSrc	= reinterpret_cast<const BufferVK*>(pSrc);
		TextureVK*		pVkDst	= reinterpret_cast<TextureVK*>(pDst);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferImageHeight				= desc.SrcHeight;
		copyRegion.bufferOffset						= desc.SrcOffset;
		copyRegion.bufferRowLength					= uint32(desc.SrcRowPitch);
		copyRegion.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT; //TODO: Other aspects
		copyRegion.imageSubresource.baseArrayLayer	= desc.ArrayIndex;
		copyRegion.imageSubresource.layerCount		= desc.ArrayCount;
		copyRegion.imageSubresource.mipLevel		= desc.Miplevel;
		copyRegion.imageExtent.depth				= desc.Depth;
		copyRegion.imageExtent.height				= desc.Height;
		copyRegion.imageExtent.width				= desc.Width;

		vkCmdCopyBufferToImage(m_CommandList, pVkSrc->GetBuffer(), pVkDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	}

	void CommandListVK::BlitTexture(const Texture* pSrc, ETextureState srcState, Texture* pDst, ETextureState dstState, EFilterType filter)
	{
		VALIDATE(pSrc != nullptr);
		VALIDATE(pDst != nullptr);

		/*const TextureVK*	pVkSrc	= reinterpret_cast<const TextureVK*>(pSrc);
		TextureVK*			pVkDst	= reinterpret_cast<TextureVK*>(pDst);

		VkImageLayout		vkSrcLayout = ConvertTextureState(srcState);
		VkImageLayout		vkDstLayout = ConvertTextureState(dstState);
		VkFilter			vkFilter	= ConvertFilter(filter);

		/*VkImageSubresourceLayers srcSubresource = {};
		srcSubresource.aspectMask;
		srcSubresource.mipLevel;
		srcSubresource.baseArrayLayer;
		srcSubresource.layerCount;

		VkImageBlit region = {};
		region.srcSubresource;
		region.srcOffsets[2];
		region.dstSubresource;
		region.dstOffsets[2];

		vkCmdBlitImage(m_CommandList, pVkSrc->GetImage(), vkSrcLayout, pVkDst->GetImage(), vkDstLayout, 1, &region, vkFilter);*/
	}

	void CommandListVK::PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrierDesc* pTextureBarriers, uint32 textureBarrierCount)
	{
		VALIDATE(pTextureBarriers		!= nullptr);
		VALIDATE(textureBarrierCount	< MAX_IMAGE_BARRIERS);

		TextureVK*		pVkTexture	= nullptr;
		VkImageLayout	oldLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout	newLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		for (uint32 i = 0; i < textureBarrierCount; i++)
		{
			const PipelineTextureBarrierDesc& barrier = pTextureBarriers[i];

			pVkTexture	= reinterpret_cast<TextureVK*>(barrier.pTexture);
			oldLayout	= ConvertTextureState(barrier.StateBefore);
			newLayout	= ConvertTextureState(barrier.StateAfter);

			m_ImageBarriers[i].sType							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			m_ImageBarriers[i].pNext							= nullptr;
			m_ImageBarriers[i].image							= pVkTexture->GetImage();
			m_ImageBarriers[i].srcQueueFamilyIndex				= m_pDevice->GetQueueFamilyIndexFromQueueType(barrier.QueueBefore);
			m_ImageBarriers[i].dstQueueFamilyIndex				= m_pDevice->GetQueueFamilyIndexFromQueueType(barrier.QueueAfter);
			m_ImageBarriers[i].oldLayout						= oldLayout;
			m_ImageBarriers[i].newLayout						= newLayout;
			m_ImageBarriers[i].subresourceRange.baseMipLevel	= barrier.Miplevel;
			m_ImageBarriers[i].subresourceRange.levelCount		= barrier.MiplevelCount;
			m_ImageBarriers[i].subresourceRange.baseArrayLayer	= barrier.ArrayIndex;
			m_ImageBarriers[i].subresourceRange.layerCount		= barrier.ArrayCount;
			m_ImageBarriers[i].srcAccessMask					= ConvertMemoryAccessFlags(barrier.SrcMemoryAccessFlags);
			m_ImageBarriers[i].dstAccessMask					= ConvertMemoryAccessFlags(barrier.DstMemoryAccessFlags);

			if (barrier.TextureFlags == FTextureFlags::TEXTURE_FLAG_DEPTH_STENCIL)
			{
				m_ImageBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else
			{
				m_ImageBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
		}

		VkPipelineStageFlags sourceStage		= ConvertPipelineStageMask(srcStage);
		VkPipelineStageFlags destinationStage	= ConvertPipelineStageMask(dstStage);
		vkCmdPipelineBarrier(m_CommandList, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, textureBarrierCount, m_ImageBarriers);
	}

	void CommandListVK::PipelineBufferBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineBufferBarrierDesc* pBufferBarriers, uint32 bufferBarrierCount)
	{
		VALIDATE(pBufferBarriers		!= nullptr);
		VALIDATE(bufferBarrierCount	< MAX_BUFFER_BARRIERS);

		BufferVK* pVkBuffer = nullptr;
		for (uint32 i = 0; i < bufferBarrierCount; i++)
		{
			const PipelineBufferBarrierDesc& barrier = pBufferBarriers[i];
			pVkBuffer = reinterpret_cast<BufferVK*>(barrier.pBuffer);

			m_BufferBarriers[i].sType				= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			m_BufferBarriers[i].pNext				= nullptr;
			m_BufferBarriers[i].buffer				= pVkBuffer->GetBuffer();
			m_BufferBarriers[i].srcQueueFamilyIndex = m_pDevice->GetQueueFamilyIndexFromQueueType(barrier.QueueBefore);
			m_BufferBarriers[i].dstQueueFamilyIndex = m_pDevice->GetQueueFamilyIndexFromQueueType(barrier.QueueAfter);
			m_BufferBarriers[i].srcAccessMask		= ConvertMemoryAccessFlags(barrier.SrcMemoryAccessFlags);
			m_BufferBarriers[i].dstAccessMask		= ConvertMemoryAccessFlags(barrier.DstMemoryAccessFlags);
			m_BufferBarriers[i].offset				= barrier.Offset;
			m_BufferBarriers[i].size				= barrier.SizeInBytes;
		}

		VkPipelineStageFlags sourceStage		= ConvertPipelineStageMask(srcStage);
		VkPipelineStageFlags destinationStage	= ConvertPipelineStageMask(dstStage);
		vkCmdPipelineBarrier(m_CommandList, sourceStage, destinationStage, 0, 0, nullptr, bufferBarrierCount, m_BufferBarriers, 0, nullptr);
	}

	void CommandListVK::GenerateMiplevels(Texture* pTexture, ETextureState stateBefore, ETextureState stateAfter)
	{
		VALIDATE(pTexture != nullptr);

		TextureVK*		pVkTexture		= reinterpret_cast<TextureVK*>(pTexture);
		TextureDesc		desc			= pVkTexture->GetDesc();
		const uint32	miplevelCount	= desc.Miplevels;
		
		if (miplevelCount < 2)
		{
			LOG_WARNING("[CommandListVK::GenerateMiplevels]: pTexture only has 1 miplevel allocated, no other mips will be generated");
			return;
		}

		constexpr uint32 REQUIRED_FLAGS = (TEXTURE_FLAG_COPY_SRC | TEXTURE_FLAG_COPY_DST);
		if ((desc.Flags & REQUIRED_FLAGS) != REQUIRED_FLAGS)
		{
			LOG_ERROR("[CommandListVK::GenerateMiplevels]: pTexture were not created with TEXTURE_FLAG_COPY_SRC and TEXTURE_FLAG_COPY_DST flags");
			DEBUGBREAK();
			return;
		}

		//TODO: Fix for devices that do NOT support linear filtering
		
		VkFormat			formatVk			= ConvertFormat(desc.Format);
		VkFormatProperties	formatProperties	= m_pDevice->GetFormatProperties(formatVk);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			LOG_ERROR("[CommandListVK]: Device does not support generating miplevels at the moment");
			return;
		}

		PipelineTextureBarrierDesc textureBarrier = { };
		textureBarrier.pTexture				= pTexture;
		textureBarrier.TextureFlags			= desc.Flags;
		textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		textureBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;

		if (stateBefore != ETextureState::TEXTURE_STATE_COPY_DST)
		{
			textureBarrier.Miplevel				= 0;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.MiplevelCount		= desc.Miplevels;
			textureBarrier.ArrayCount			= desc.ArrayCount;
			textureBarrier.SrcMemoryAccessFlags = 0;
			textureBarrier.DstMemoryAccessFlags = FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			textureBarrier.StateBefore			= stateBefore;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_COPY_DST;
			PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, &textureBarrier, 1);
		}

		VkImage		imageVk				= pVkTexture->GetImage();
		VkExtent2D	destinationExtent	= {};
		VkExtent2D	sourceExtent		= { desc.Width, desc.Height };
		for (uint32_t i = 1; i < miplevelCount; i++)
		{
			destinationExtent = { std::max(sourceExtent.width / 2U, 1u), std::max(sourceExtent.height / 2U, 1U) };

			textureBarrier.Miplevel				= i - 1;
			textureBarrier.MiplevelCount		= 1;
			textureBarrier.ArrayCount			= 1;
			textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_COPY_DST;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_COPY_SRC;
			textureBarrier.SrcMemoryAccessFlags = FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			textureBarrier.DstMemoryAccessFlags = FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
			PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, &textureBarrier, 1);

			VkImageBlit blit = {};
			blit.srcOffsets[0]					= { 0, 0, 0 };
			blit.srcOffsets[1]					= { int32_t(sourceExtent.width), int32_t(sourceExtent.height), int32_t(1) };
			blit.srcSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel		= i - 1;
			blit.srcSubresource.baseArrayLayer	= 0;
			blit.srcSubresource.layerCount		= 1;
			blit.dstOffsets[0]					= { 0, 0, 0 };
			blit.dstOffsets[1]					= { int32_t(destinationExtent.width), int32_t(destinationExtent.height), int32_t(1) };
			blit.dstSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel		= i;
			blit.dstSubresource.baseArrayLayer	= 0;
			blit.dstSubresource.layerCount		= 1;

			vkCmdBlitImage(m_CommandList, imageVk, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imageVk, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

			sourceExtent = destinationExtent;
		}

		textureBarrier.Miplevel = miplevelCount - 1;
		PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, &textureBarrier, 1);

		if (stateAfter != ETextureState::TEXTURE_STATE_COPY_SRC)
		{
			textureBarrier.Miplevel			= 0;
			textureBarrier.MiplevelCount	= desc.Miplevels;
			textureBarrier.ArrayCount		= desc.ArrayCount;
			textureBarrier.StateBefore		= ETextureState::TEXTURE_STATE_COPY_SRC;
			textureBarrier.StateAfter		= stateAfter;
			PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM, &textureBarrier, 1);
		}
	}

	void CommandListVK::SetViewports(const Viewport* pViewports, uint32 firstViewport, uint32 viewportCount)
	{
		for (uint32 i = firstViewport; i < viewportCount; i++)
		{
			const Viewport&     viewport    = pViewports[i];
			VkViewport&         viewportVk  = m_Viewports[i];
			
			viewportVk.width    = viewport.Width;
			viewportVk.height   = viewport.Height;
			viewportVk.minDepth = viewport.MinDepth;
			viewportVk.maxDepth = viewport.MaxDepth;
			viewportVk.x        = viewport.x;
			viewportVk.y        = viewport.y;
		}
		
		vkCmdSetViewport(m_CommandList, firstViewport, viewportCount, m_Viewports);
	}

	void CommandListVK::SetScissorRects(const ScissorRect* pScissorRects, uint32 firstScissor, uint32 scissorCount)
	{
		for (uint32 i = firstScissor; i < scissorCount; i++)
		{
			const ScissorRect& scissorRect    = pScissorRects[i];
			VkRect2D&          scissorRectVk  = m_ScissorRects[i];
			
			scissorRectVk.extent   = { scissorRect.Width, scissorRect.Height };
			scissorRectVk.offset   = { scissorRect.x, scissorRect.y };
		}
		
		vkCmdSetScissor(m_CommandList, firstScissor, scissorCount, m_ScissorRects);
	}

	void CommandListVK::SetConstantRange(const PipelineLayout* pPipelineLayout, uint32 shaderStageMask, const void* pConstants, uint32 size, uint32 offset)
	{
		const PipelineLayoutVK* pVkPipelineLayout = reinterpret_cast<const PipelineLayoutVK*>(pPipelineLayout);
		uint32 shaderStageMaskVk = ConvertShaderStageMask(shaderStageMask);

		vkCmdPushConstants(m_CommandList, pVkPipelineLayout->GetPipelineLayout(), shaderStageMaskVk, offset, size, pConstants);
	}

	void CommandListVK::BindIndexBuffer(const Buffer* pIndexBuffer, uint64 offset, EIndexType indexType)
	{
		const BufferVK* pIndexBufferVK = reinterpret_cast<const BufferVK*>(pIndexBuffer);
		vkCmdBindIndexBuffer(m_CommandList, pIndexBufferVK->GetBuffer(), offset, ConvertIndexType(indexType));
	}

	void CommandListVK::BindVertexBuffers(const Buffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount)
	{
		for (uint32 i = 0; i < vertexBufferCount; i++)
		{
			const BufferVK* pVertexBufferVK = reinterpret_cast<const BufferVK*>(ppVertexBuffers[i]);
			
			m_VertexBuffers[i]          = pVertexBufferVK->GetBuffer();
			m_VertexBufferOffsets[i]    = VkDeviceSize(pOffsets[i]);
		}
		
		vkCmdBindVertexBuffers(m_CommandList, firstBuffer, vertexBufferCount, m_VertexBuffers, m_VertexBufferOffsets);
	}

	void CommandListVK::BindDescriptorSetGraphics(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex)
	{
		CHECK_GRAPHICS(m_Allocator);
		BindDescriptorSet(pDescriptorSet, pPipelineLayout, setIndex, VK_PIPELINE_BIND_POINT_GRAPHICS);
	}

	void CommandListVK::BindDescriptorSetCompute(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex)
	{
		CHECK_COMPUTE(m_Allocator);
		BindDescriptorSet(pDescriptorSet, pPipelineLayout, setIndex, VK_PIPELINE_BIND_POINT_COMPUTE);
	}

	void CommandListVK::BindDescriptorSetRayTracing(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex)
	{
		CHECK_COMPUTE(m_Allocator);
		BindDescriptorSet(pDescriptorSet, pPipelineLayout, setIndex, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);
	}

	void CommandListVK::BindGraphicsPipeline(const PipelineState* pPipeline)
	{
		CHECK_GRAPHICS(m_Allocator);
		VALIDATE(pPipeline->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS);
		
		const GraphicsPipelineStateVK* pPipelineVk = reinterpret_cast<const GraphicsPipelineStateVK*>(pPipeline);
		vkCmdBindPipeline(m_CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineVk->GetPipeline());
	}

	void CommandListVK::BindComputePipeline(const PipelineState* pPipeline)
	{
		CHECK_COMPUTE(m_Allocator);
		VALIDATE(pPipeline->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE);
		
		const ComputePipelineStateVK* pPipelineVk = reinterpret_cast<const ComputePipelineStateVK*>(pPipeline);
		vkCmdBindPipeline(m_CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineVk->GetPipeline());
	}

	void CommandListVK::BindRayTracingPipeline(PipelineState* pPipeline)
	{
		CHECK_COMPUTE(m_Allocator);
		VALIDATE(pPipeline->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING);
		
		m_CurrentRayTracingPipeline = reinterpret_cast<RayTracingPipelineStateVK*>(pPipeline);
		vkCmdBindPipeline(m_CommandList, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_CurrentRayTracingPipeline->GetPipeline());
	}

	void CommandListVK::TraceRays(uint32 width, uint32 height, uint32 depth)
	{
		CHECK_COMPUTE(m_Allocator);

		if (m_pDevice->vkCmdTraceRaysKHR)
		{
			const BufferVK* pShaderBindingTableVk = m_CurrentRayTracingPipeline->GetShaderBindingTable();
			
			VkStridedBufferRegionKHR rayGen = {};
			rayGen.buffer   = pShaderBindingTableVk->GetBuffer();
			rayGen.offset   = m_CurrentRayTracingPipeline->GetBindingOffsetRaygenGroup();
			rayGen.stride   = m_CurrentRayTracingPipeline->GetBindingStride();
			rayGen.size     = m_CurrentRayTracingPipeline->GetBindingSizeRaygenGroup();

			VkStridedBufferRegionKHR miss = {};
			miss.buffer   = pShaderBindingTableVk->GetBuffer();
			miss.offset   = m_CurrentRayTracingPipeline->GetBindingOffsetMissGroup();
			miss.stride   = rayGen.stride;
			miss.size     = m_CurrentRayTracingPipeline->GetBindingSizeMissGroup();
			
			VkStridedBufferRegionKHR hit = {};
			hit.buffer   = pShaderBindingTableVk->GetBuffer();
			hit.offset   = m_CurrentRayTracingPipeline->GetBindingOffsetHitGroup();
			hit.stride   = rayGen.stride;
			hit.size     = m_CurrentRayTracingPipeline->GetBindingSizeHitGroup();
			
			VkStridedBufferRegionKHR callable = {};
			callable.buffer   = VK_NULL_HANDLE;
			callable.offset   = 0;
			callable.stride   = 0;
			callable.size     = 0;
			
			m_pDevice->vkCmdTraceRaysKHR(m_CommandList, &rayGen, &miss, &hit, &callable, width, height, depth);
		}
	}

	void CommandListVK::Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ)
	{
		CHECK_COMPUTE(m_Allocator);
		vkCmdDispatch(m_CommandList, workGroupCountX, workGroupCountY, workGroupCountZ);
	}

	void CommandListVK::DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
		CHECK_GRAPHICS(m_Allocator);
		vkCmdDraw(m_CommandList, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandListVK::DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)
	{
		CHECK_GRAPHICS(m_Allocator);
		vkCmdDrawIndexed(m_CommandList, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandListVK::DrawIndexedIndirect(const Buffer* pDrawBuffer, uint32 offset, uint32 drawCount, uint32 stride)
	{
		const BufferVK* pDrawBufferVk = reinterpret_cast<const BufferVK*>(pDrawBuffer);
		vkCmdDrawIndexedIndirect(m_CommandList, pDrawBufferVk->GetBuffer(), offset, drawCount, stride);
	}

	void CommandListVK::BeginQuery(QueryHeap* pQueryHeap, uint32 queryIndex)
	{
		QueryHeapVK*		pQueryHeapVk	= reinterpret_cast<QueryHeapVK*>(pQueryHeap);
		VkQueryControlFlags controlFlagsVk	= VK_QUERY_CONTROL_PRECISE_BIT;
		vkCmdBeginQuery(m_CommandList, pQueryHeapVk->GetQueryPool(), queryIndex, controlFlagsVk);
	}

	void CommandListVK::Timestamp(QueryHeap* pQueryHeap, uint32 queryIndex, FPipelineStageFlags pipelineStageFlag)
	{
		QueryHeapVK*			pQueryHeapVk		= reinterpret_cast<QueryHeapVK*>(pQueryHeap);
		VkPipelineStageFlagBits	pipelineStageFlagVk	= ConvertPipelineStage(pipelineStageFlag);
		vkCmdWriteTimestamp(m_CommandList, pipelineStageFlagVk, pQueryHeapVk->GetQueryPool(), queryIndex);
	}

	void CommandListVK::EndQuery(QueryHeap* pQueryHeap, uint32 queryIndex)
	{
		QueryHeapVK* pQueryHeapVk = reinterpret_cast<QueryHeapVK*>(pQueryHeap);
		vkCmdEndQuery(m_CommandList, pQueryHeapVk->GetQueryPool(), queryIndex);
	}

	void CommandListVK::ExecuteSecondary(const CommandList* pSecondary)
	{
		VALIDATE(m_Desc.CommandListType == ECommandListType::COMMAND_LIST_TYPE_PRIMARY);
		const CommandListVK* pVkSecondary = reinterpret_cast<const CommandListVK*>(pSecondary);

#ifndef LAMBDA_DISABLE_ASSERTS 
		CommandListDesc	desc = pVkSecondary->GetDesc();
		VALIDATE(desc.CommandListType == ECommandListType::COMMAND_LIST_TYPE_SECONDARY);
#endif

		vkCmdExecuteCommands(m_CommandList, 1, &pVkSecondary->m_CommandList);
	}
	
	CommandAllocator* CommandListVK::GetAllocator()
	{
		return m_Allocator.GetAndAddRef();
	}
	
	FORCEINLINE void CommandListVK::BindDescriptorSet(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex, VkPipelineBindPoint bindPoint)
	{
		const PipelineLayoutVK* pVkPipelineLayout	= reinterpret_cast<const PipelineLayoutVK*>(pPipelineLayout);
		const DescriptorSetVK* pVkDescriptorSet		= reinterpret_cast<const DescriptorSetVK*>(pDescriptorSet);

		VkDescriptorSet descriptorSet = pVkDescriptorSet->GetDescriptorSet();
		vkCmdBindDescriptorSets(m_CommandList, bindPoint, pVkPipelineLayout->GetPipelineLayout(), setIndex, 1, &descriptorSet, 0, nullptr);
	}
}
