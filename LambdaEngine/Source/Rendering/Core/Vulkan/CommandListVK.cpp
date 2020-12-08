#include "Log/Log.h"

#include <algorithm>

#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/ComputePipelineStateVK.h"
#include "Rendering/Core/Vulkan/GraphicsPipelineStateVK.h"
#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/RenderPassVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/DescriptorSetVK.h"
#include "Rendering/Core/Vulkan/QueryHeapVK.h"
#include "Rendering/Core/Vulkan/AccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/SBTVK.h"

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
		: TDeviceChild(pDevice)
		, m_MemoryBarriers()
		, m_BufferBarriers()
		, m_ImageBarriers()
		, m_Viewports()
		, m_ScissorRects()
		, m_VertexBuffers()
		, m_VertexBufferOffsets()
	{
	}

	CommandListVK::~CommandListVK()
	{
		// There has to be an allocator for the commandlist to be valid
		VALIDATE(m_Allocator != nullptr);
		m_Allocator->FreeCommandBuffer(m_CmdBuffer);

		m_CmdBuffer = VK_NULL_HANDLE;
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

		CommandAllocatorVK*	pVkCommandAllocator = reinterpret_cast<CommandAllocatorVK*>(pAllocator);
		m_CmdBuffer = pVkCommandAllocator->AllocateCommandBuffer(level);
		if (m_CmdBuffer != VK_NULL_HANDLE)
		{
			m_Desc		= *pDesc;
			m_QueueType = pAllocator->GetType();
			m_Allocator = pVkCommandAllocator;
			m_Allocator->AddRef();
			SetName(pDesc->DebugName);

			return true;
		}
		else
		{
			return false;
		}
	}

	void CommandListVK::SetName(const String& debugname)
	{
			m_pDevice->SetVulkanObjectName(debugname, reinterpret_cast<uint64>(m_CmdBuffer), VK_OBJECT_TYPE_COMMAND_BUFFER);
			m_Desc.DebugName = debugname;
	}

	bool CommandListVK::Begin(const SecondaryCommandListBeginDesc* pBeginDesc)
	{
		//Destroy all deferred resources before beginning
		FlushDeferredResources();

		// Begin CommandList
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags	= 0;
		if (m_Desc.Flags & FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT)
		{
			beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		}

		VkCommandBufferInheritanceInfo inheritanceInfo = { };
		if (pBeginDesc)
		{
			const RenderPassVK*	pRenderPassVk = reinterpret_cast<const RenderPassVK*>(pBeginDesc->pRenderPass);

			VALIDATE(pRenderPassVk != nullptr);
			
			inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritanceInfo.pNext = nullptr;

			if (pBeginDesc->ppRenderTargets != nullptr || pBeginDesc->pDepthStencilView != nullptr)
			{
				inheritanceInfo.framebuffer = m_pDevice->GetFrameBuffer(pRenderPassVk, pBeginDesc->ppRenderTargets, pBeginDesc->RenderTargetCount, pBeginDesc->pDepthStencilView, pBeginDesc->Width, pBeginDesc->Height);
			}
			else
			{
				inheritanceInfo.framebuffer = VK_NULL_HANDLE;
			}

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

		VkResult result = vkBeginCommandBuffer(m_CmdBuffer, &beginInfo);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandListVK]: Begin CommandBuffer Failed");
			return false;
		}
		else
		{
			m_IsRecording = true;
			return true;
		}
	}

	bool CommandListVK::End()
	{
		// Make sure that all deferred barriers gets subited to the commanslist before ending
		FlushDeferredBarriers();

		// End CommandBuffer
		VkResult result = vkEndCommandBuffer(m_CmdBuffer);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandListVK]: End CommandBuffer Failed");
			return false;
		}
		else
		{
			m_IsRecording = false;
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
		if (pBeginDesc->Flags & FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_EXECUTE_SECONDARY)
		{
			subpassContent = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
		}

		// Start by flushing barriers
		FlushDeferredBarriers();

		vkCmdBeginRenderPass(m_CmdBuffer, &renderPassInfo, subpassContent);
	}

	void CommandListVK::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CmdBuffer);
	}

	void CommandListVK::BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* pBuildDesc)
	{
		VALIDATE(pBuildDesc != nullptr);

		VALIDATE(pBuildDesc->pAccelerationStructure != nullptr);
		VALIDATE(pBuildDesc->pInstanceBuffer		!= nullptr);
		AccelerationStructureVK* pAccelerationStructureVk	= reinterpret_cast<AccelerationStructureVK*>(pBuildDesc->pAccelerationStructure);
		BufferVK*		pScratchBufferVk					= pAccelerationStructureVk->GetScratchBuffer();
		const BufferVK* pInstanceBufferVk					= reinterpret_cast<const BufferVK*>(pBuildDesc->pInstanceBuffer);

		VkAccelerationStructureGeometryKHR geometryData = {};
		geometryData.sType									= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometryData.flags									= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
		geometryData.geometryType							= VK_GEOMETRY_TYPE_INSTANCES_KHR;
		geometryData.geometry.instances.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		geometryData.geometry.instances.arrayOfPointers		= VK_FALSE;
		geometryData.geometry.instances.data.deviceAddress	= pInstanceBufferVk->GetDeviceAddress();

		VkAccelerationStructureGeometryKHR* pGeometryData = &geometryData;

		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildInfo = {};
		accelerationStructureBuildInfo.sType						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildInfo.type							= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildInfo.flags						= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		accelerationStructureBuildInfo.geometryArrayOfPointers		= VK_FALSE;
		accelerationStructureBuildInfo.geometryCount				= 1;
		accelerationStructureBuildInfo.ppGeometries					= &pGeometryData;
		accelerationStructureBuildInfo.scratchData.deviceAddress	= pScratchBufferVk->GetDeviceAddress();

		//Extra Flags
		{
			if (pBuildDesc->Flags & FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE)
			{
				accelerationStructureBuildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
			}
		}

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
		accelerationStructureOffsetInfo.primitiveCount		= pBuildDesc->InstanceCount;
		accelerationStructureOffsetInfo.primitiveOffset		= 0x0;
		accelerationStructureOffsetInfo.firstVertex			= 0;
		accelerationStructureOffsetInfo.transformOffset		= 0x0;

		VALIDATE(m_pDevice->vkCmdBuildAccelerationStructureKHR != nullptr);

		// Start by flushing barriers
		FlushDeferredBarriers();

		VkAccelerationStructureBuildOffsetInfoKHR* pAccelerationStructureOffsetInfo = &accelerationStructureOffsetInfo;
		m_pDevice->vkCmdBuildAccelerationStructureKHR(m_CmdBuffer, 1, &accelerationStructureBuildInfo, &pAccelerationStructureOffsetInfo);
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

		VkAccelerationStructureGeometryKHR geometryData = {};
		geometryData.sType											= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometryData.flags											= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR; //VK_GEOMETRY_OPAQUE_BIT_KHR
		geometryData.geometryType									= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometryData.geometry.triangles.sType						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometryData.geometry.triangles.vertexFormat				= VK_FORMAT_R32G32B32_SFLOAT;
		geometryData.geometry.triangles.vertexData.deviceAddress	= pVertexBufferVk->GetDeviceAddress();
		geometryData.geometry.triangles.vertexStride				= pBuildDesc->VertexStride;
		geometryData.geometry.triangles.indexType					= VK_INDEX_TYPE_UINT32;
		geometryData.geometry.triangles.indexData.deviceAddress		= pIndexBufferVk->GetDeviceAddress();

		if (pBuildDesc->pTransformBuffer != nullptr)
		{
			const BufferVK* pTransformBufferVk = reinterpret_cast<const BufferVK*>(pBuildDesc->pTransformBuffer);
			geometryData.geometry.triangles.transformData.deviceAddress = pTransformBufferVk->GetDeviceAddress();
		}

		VkAccelerationStructureGeometryKHR* pGeometryData = &geometryData;

		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildInfo = {};
		accelerationStructureBuildInfo.sType						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildInfo.type							= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildInfo.flags						= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		accelerationStructureBuildInfo.geometryArrayOfPointers		= VK_FALSE;
		accelerationStructureBuildInfo.geometryCount				= 1;
		accelerationStructureBuildInfo.ppGeometries					= &pGeometryData;
		accelerationStructureBuildInfo.scratchData.deviceAddress	= pScratchBufferVk->GetDeviceAddress();

		//Extra Flags
		{
			if (pBuildDesc->Flags & FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE)
			{
				accelerationStructureBuildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
			}
		}

		if (pBuildDesc->Update)
		{
			accelerationStructureBuildInfo.update					= VK_TRUE;
			accelerationStructureBuildInfo.srcAccelerationStructure	= pAccelerationStructureVk->GetAccelerationStructure();
			accelerationStructureBuildInfo.dstAccelerationStructure	= pAccelerationStructureVk->GetAccelerationStructure();
		}
		else
		{
			accelerationStructureBuildInfo.update					= VK_FALSE;
			accelerationStructureBuildInfo.srcAccelerationStructure	= VK_NULL_HANDLE;
			accelerationStructureBuildInfo.dstAccelerationStructure	= pAccelerationStructureVk->GetAccelerationStructure();
		}

		VkAccelerationStructureBuildOffsetInfoKHR accelerationStructureOffsetInfo = {};
		accelerationStructureOffsetInfo.primitiveCount	= pBuildDesc->TriangleCount;
		accelerationStructureOffsetInfo.primitiveOffset = pBuildDesc->IndexBufferByteOffset;
		accelerationStructureOffsetInfo.firstVertex		= pBuildDesc->FirstVertexIndex;
		accelerationStructureOffsetInfo.transformOffset = pBuildDesc->TransformByteOffset;

		VALIDATE(m_pDevice->vkCmdBuildAccelerationStructureKHR != nullptr);

		// Start by flushing barriers
		FlushDeferredBarriers();

		VkAccelerationStructureBuildOffsetInfoKHR* pAccelerationStructureOffsetInfo = &accelerationStructureOffsetInfo;
		m_pDevice->vkCmdBuildAccelerationStructureKHR(m_CmdBuffer, 1, &accelerationStructureBuildInfo, &pAccelerationStructureOffsetInfo);
	}

	void CommandListVK::ClearColorTexture(
		Texture* pTexture, 
		ETextureState textureState, 
		const float32 color[4])
	{
		VALIDATE(pTexture != nullptr);

		TextureVK* pVkTexture = static_cast<TextureVK*>(pTexture);

		VkClearColorValue colorValue;
		memcpy(colorValue.float32, color, sizeof(color));

		VkImageSubresourceRange range;
		range.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseArrayLayer	= 0;
		range.layerCount		= VK_REMAINING_ARRAY_LAYERS;
		range.baseMipLevel		= 0;
		range.levelCount		= VK_REMAINING_MIP_LEVELS;

		FlushDeferredBarriers();

		VkImageLayout imageLayout = ConvertTextureState(textureState);
		vkCmdClearColorImage(m_CmdBuffer, pVkTexture->GetImage(), imageLayout, &colorValue, 1, &range);
	}

	void CommandListVK::CopyBuffer(
		const Buffer* pSrc, 
		uint64 srcOffset, 
		Buffer* pDst, 
		uint64 dstOffset, 
		uint64 sizeInBytes)
	{
		VALIDATE(pSrc != nullptr);
		VALIDATE(pDst != nullptr);

		const BufferVK* pVkSrc	= reinterpret_cast<const BufferVK*>(pSrc);
		BufferVK*		pVkDst	= reinterpret_cast<BufferVK*>(pDst);

		VkBufferCopy bufferCopy = {};
		bufferCopy.size			= sizeInBytes;
		bufferCopy.srcOffset	= srcOffset;
		bufferCopy.dstOffset	= dstOffset;

		// Start by flushing barriers
		FlushDeferredBarriers();

		vkCmdCopyBuffer(m_CmdBuffer, pVkSrc->GetBuffer(), pVkDst->GetBuffer(), 1, &bufferCopy);
	}

	void CommandListVK::CopyTextureFromBuffer(const Buffer* pSrc, Texture* pDst, const CopyTextureBufferDesc& desc)
	{
		VALIDATE(pSrc != nullptr);
		VALIDATE(pDst != nullptr);

		const BufferVK* pVkSrc	= reinterpret_cast<const BufferVK*>(pSrc);
		TextureVK*		pVkDst	= reinterpret_cast<TextureVK*>(pDst);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferImageHeight				= desc.BufferHeight;
		copyRegion.bufferOffset						= desc.BufferOffset;
		copyRegion.bufferRowLength					= uint32(desc.BufferRowPitch);
		copyRegion.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT; //TODO: Other aspects
		copyRegion.imageSubresource.baseArrayLayer	= desc.ArrayIndex;
		copyRegion.imageSubresource.layerCount		= desc.ArrayCount;
		copyRegion.imageSubresource.mipLevel		= desc.Miplevel;
		copyRegion.imageOffset.x					= desc.OffsetX;
		copyRegion.imageOffset.y					= desc.OffsetY;
		copyRegion.imageOffset.z					= desc.OffsetZ;
		copyRegion.imageExtent.depth				= desc.Depth;
		copyRegion.imageExtent.height				= desc.Height;
		copyRegion.imageExtent.width				= desc.Width;

		// Start by flushing barriers
		FlushDeferredBarriers();

		vkCmdCopyBufferToImage(m_CmdBuffer, pVkSrc->GetBuffer(), pVkDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	}

	void CommandListVK::CopyTextureToBuffer(const Texture* pSrc, Buffer* pDst, const CopyTextureBufferDesc& desc)
	{
		VALIDATE(pSrc != nullptr);
		VALIDATE(pDst != nullptr);

		BufferVK*			pVkDst = reinterpret_cast<BufferVK*>(pDst);
		const TextureVK*	pVkSrc = reinterpret_cast<const TextureVK*>(pSrc);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferImageHeight				= desc.BufferHeight;
		copyRegion.bufferOffset						= desc.BufferOffset;
		copyRegion.bufferRowLength					= uint32(desc.BufferRowPitch);
		copyRegion.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT; //TODO: Other aspects
		copyRegion.imageSubresource.baseArrayLayer	= desc.ArrayIndex;
		copyRegion.imageSubresource.layerCount		= desc.ArrayCount;
		copyRegion.imageSubresource.mipLevel		= desc.Miplevel;
		copyRegion.imageOffset.x					= desc.OffsetX;
		copyRegion.imageOffset.y					= desc.OffsetY;
		copyRegion.imageOffset.z					= desc.OffsetZ;
		copyRegion.imageExtent.depth				= desc.Depth;
		copyRegion.imageExtent.height				= desc.Height;
		copyRegion.imageExtent.width				= desc.Width;

		// Start by flushing barriers
		FlushDeferredBarriers();

		vkCmdCopyImageToBuffer(m_CmdBuffer, pVkSrc->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pVkDst->GetBuffer(), 1, &copyRegion);
	}

	void CommandListVK::BlitTexture(const Texture* pSrc, ETextureState srcState, const Texture* pDst, ETextureState dstState, EFilterType filter)
	{
		VALIDATE(pSrc != nullptr);
		VALIDATE(pDst != nullptr);

		UNREFERENCED_VARIABLE(srcState);
		UNREFERENCED_VARIABLE(dstState);
		UNREFERENCED_VARIABLE(filter);

		const TextureDesc& srcTextureDesc = pSrc->GetDesc();
		const TextureDesc& dstTextureDesc = pDst->GetDesc();

		VALIDATE(srcTextureDesc.ArrayCount == dstTextureDesc.ArrayCount);

		const TextureVK*	pVkSrc	= reinterpret_cast<const TextureVK*>(pSrc);
		const TextureVK*	pVkDst	= reinterpret_cast<const TextureVK*>(pDst);

		VALIDATE(pVkSrc->GetAspectFlags() == pVkDst->GetAspectFlags());

		VkImageLayout		vkSrcLayout = ConvertTextureState(srcState);
		VkImageLayout		vkDstLayout = ConvertTextureState(dstState);
		VkFilter			vkFilter	= ConvertFilter(filter);

		VkImageSubresourceLayers srcSubresource = {};
		
		static VkImageBlit region = {};
		region.srcSubresource.aspectMask		= pVkSrc->GetAspectFlags();
		region.srcSubresource.mipLevel			= 0;
		region.srcSubresource.baseArrayLayer	= 0;
		region.srcSubresource.layerCount		= srcTextureDesc.ArrayCount;
		region.srcOffsets[1].x					= srcTextureDesc.Width;
		region.srcOffsets[1].y					= srcTextureDesc.Height;
		region.srcOffsets[1].z					= 1;
		region.dstSubresource.aspectMask		= pVkDst->GetAspectFlags();
		region.dstSubresource.mipLevel			= 0;
		region.dstSubresource.baseArrayLayer	= 0;
		region.dstSubresource.layerCount		= dstTextureDesc.ArrayCount;
		region.dstOffsets[1].x					= dstTextureDesc.Width;
		region.dstOffsets[1].y					= dstTextureDesc.Height;
		region.dstOffsets[1].z					= 1;

		// Start by flushing barriers
		FlushDeferredBarriers();

		if (srcState != ETextureState::TEXTURE_STATE_COPY_SRC)
		{
			VkImageMemoryBarrier imageBarrier = { };
			imageBarrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext								= nullptr;
			imageBarrier.image								= pVkSrc->GetImage();
			imageBarrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.oldLayout							= vkSrcLayout;
			imageBarrier.newLayout							= VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.subresourceRange.baseMipLevel		= 0;
			imageBarrier.subresourceRange.levelCount		= VK_REMAINING_MIP_LEVELS;
			imageBarrier.subresourceRange.baseArrayLayer	= region.srcSubresource.baseArrayLayer;
			imageBarrier.subresourceRange.layerCount		= region.srcSubresource.layerCount;
			imageBarrier.subresourceRange.aspectMask		= region.srcSubresource.aspectMask;
			imageBarrier.srcAccessMask						= VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;
			imageBarrier.dstAccessMask						= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;

			vkCmdPipelineBarrier(m_CmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
		}

		if (dstState != ETextureState::TEXTURE_STATE_COPY_SRC)
		{
			VkImageMemoryBarrier imageBarrier = { };
			imageBarrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext								= nullptr;
			imageBarrier.image								= pVkDst->GetImage();
			imageBarrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.oldLayout							= vkDstLayout;
			imageBarrier.newLayout							= VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.subresourceRange.baseMipLevel		= 0;
			imageBarrier.subresourceRange.levelCount		= VK_REMAINING_MIP_LEVELS;
			imageBarrier.subresourceRange.baseArrayLayer	= region.dstSubresource.baseArrayLayer;
			imageBarrier.subresourceRange.layerCount		= region.dstSubresource.layerCount;
			imageBarrier.subresourceRange.aspectMask		= region.dstSubresource.aspectMask;
			imageBarrier.srcAccessMask						= VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;
			imageBarrier.dstAccessMask						= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;

			vkCmdPipelineBarrier(m_CmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
		}

		vkCmdBlitImage(m_CmdBuffer, pVkSrc->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pVkDst->GetImage(), VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, vkFilter);

		if (srcState != ETextureState::TEXTURE_STATE_COPY_SRC)
		{
			VkImageMemoryBarrier imageBarrier = { };
			imageBarrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext								= nullptr;
			imageBarrier.image								= pVkSrc->GetImage();
			imageBarrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.oldLayout							= VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.newLayout							= vkSrcLayout;
			imageBarrier.subresourceRange.baseMipLevel		= 0;
			imageBarrier.subresourceRange.levelCount		= VK_REMAINING_MIP_LEVELS;
			imageBarrier.subresourceRange.baseArrayLayer	= region.srcSubresource.baseArrayLayer;
			imageBarrier.subresourceRange.layerCount		= region.srcSubresource.layerCount;
			imageBarrier.subresourceRange.aspectMask		= region.srcSubresource.aspectMask;
			imageBarrier.srcAccessMask						= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
			imageBarrier.dstAccessMask						= VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;

			vkCmdPipelineBarrier(m_CmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
		}

		if (dstState != ETextureState::TEXTURE_STATE_COPY_SRC)
		{
			VkImageMemoryBarrier imageBarrier = { };
			imageBarrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext								= nullptr;
			imageBarrier.image								= pVkDst->GetImage();
			imageBarrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.oldLayout							= VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.newLayout							= vkDstLayout;
			imageBarrier.subresourceRange.baseMipLevel		= 0;
			imageBarrier.subresourceRange.levelCount		= VK_REMAINING_MIP_LEVELS;
			imageBarrier.subresourceRange.baseArrayLayer	= region.dstSubresource.baseArrayLayer;
			imageBarrier.subresourceRange.layerCount		= region.dstSubresource.layerCount;
			imageBarrier.subresourceRange.aspectMask		= region.dstSubresource.aspectMask;
			imageBarrier.srcAccessMask						= VkAccessFlagBits::VK_ACCESS_MEMORY_READ_BIT;
			imageBarrier.dstAccessMask						= VkAccessFlagBits::VK_ACCESS_MEMORY_WRITE_BIT;

			vkCmdPipelineBarrier(m_CmdBuffer, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT, VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
		}
	}

	void CommandListVK::TransitionBarrier(
		Texture* pTexture, 
		FPipelineStageFlags srcStage, 
		FPipelineStageFlags dstStage, 
		uint32 srcAccessMask, 
		uint32 destAccessMask,
		ETextureState beforeState, 
		ETextureState afterState)
	{
		TransitionBarrier(pTexture, srcStage, dstStage, srcAccessMask, destAccessMask, 0, VK_REMAINING_ARRAY_LAYERS, beforeState, afterState);
	}

	void CommandListVK::TransitionBarrier(
		Texture* pTexture, 
		FPipelineStageFlags srcStage, 
		FPipelineStageFlags dstStage, 
		uint32 srcAccessMask, 
		uint32 destAccessMask,
		uint32 arrayIndex, 
		uint32 arrayCount, 
		ETextureState beforeState, 
		ETextureState afterState)
	{
		TextureVK* pTextureVk = reinterpret_cast<TextureVK*>(pTexture);

		// Create barrier
		VkImageMemoryBarrier imageBarrier = { };
		imageBarrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.pNext								= nullptr;
		imageBarrier.image								= pTextureVk->GetImage();
		imageBarrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.oldLayout							= ConvertTextureState(beforeState);
		imageBarrier.newLayout							= ConvertTextureState(afterState);
		imageBarrier.subresourceRange.baseMipLevel		= 0;
		imageBarrier.subresourceRange.levelCount		= VK_REMAINING_MIP_LEVELS;
		imageBarrier.subresourceRange.baseArrayLayer	= arrayIndex;
		imageBarrier.subresourceRange.layerCount		= arrayCount;
		imageBarrier.subresourceRange.aspectMask		= pTextureVk->GetAspectFlags();
		imageBarrier.srcAccessMask						= ConvertMemoryAccessFlags(srcAccessMask);
		imageBarrier.dstAccessMask						= ConvertMemoryAccessFlags(destAccessMask);

		// Find suitable barrier batch
		DeferredImageBarrier deferredBarrier;
		deferredBarrier.SrcStages	= ConvertPipelineStageMask(srcStage);
		deferredBarrier.DestStages	= ConvertPipelineStageMask(dstStage);
		AddDeferredBarrier(deferredBarrier, imageBarrier);
	}

	void CommandListVK::QueueTransferBarrier(
		Texture* pTexture, 
		FPipelineStageFlags srcStage, 
		FPipelineStageFlags dstStage, 
		uint32 srcAccessMask, 
		uint32 destAccessMask, 
		ECommandQueueType srcQueue, 
		ECommandQueueType dstQueue, 
		ETextureState beforeState, 
		ETextureState afterState)
	{
		TextureVK* pTextureVk = reinterpret_cast<TextureVK*>(pTexture);
		const TextureDesc& desc = pTextureVk->GetDesc();

		// Create barrier
		VkImageMemoryBarrier imageBarrier = { };
		imageBarrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.pNext								= nullptr;
		imageBarrier.image								= pTextureVk->GetImage();
		imageBarrier.srcQueueFamilyIndex				= m_pDevice->GetQueueFamilyIndexFromQueueType(srcQueue);
		imageBarrier.dstQueueFamilyIndex				= m_pDevice->GetQueueFamilyIndexFromQueueType(dstQueue);
		imageBarrier.oldLayout							= ConvertTextureState(beforeState);
		imageBarrier.newLayout							= ConvertTextureState(afterState);
		imageBarrier.subresourceRange.baseMipLevel		= 0;
		imageBarrier.subresourceRange.levelCount		= desc.Miplevels;
		imageBarrier.subresourceRange.baseArrayLayer	= 0;
		imageBarrier.subresourceRange.layerCount		= desc.ArrayCount;
		imageBarrier.srcAccessMask						= ConvertMemoryAccessFlags(srcAccessMask);
		imageBarrier.dstAccessMask						= ConvertMemoryAccessFlags(destAccessMask);

		if (desc.Flags == FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL)
		{
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else
		{
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		// Find suitable barrier batch
		DeferredImageBarrier deferredBarrier;
		deferredBarrier.SrcStages	= ConvertPipelineStageMask(srcStage);
		deferredBarrier.DestStages	= ConvertPipelineStageMask(dstStage);
		AddDeferredBarrier(deferredBarrier, imageBarrier);
	}

	void CommandListVK::PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrierDesc* pTextureBarriers, uint32 textureBarrierCount)
	{
		VALIDATE(pTextureBarriers		!= nullptr);
		VALIDATE(textureBarrierCount	<= MAX_IMAGE_BARRIERS);

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

			if ((barrier.TextureFlags & FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL) > 0)
			{
				m_ImageBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			else
			{
				m_ImageBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
		}

		VkPipelineStageFlags sourceStage		= ConvertPipelineStageMask(srcStage);
		VkPipelineStageFlags destinationStage	= ConvertPipelineStageMask(dstStage);
		vkCmdPipelineBarrier(m_CmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, textureBarrierCount, m_ImageBarriers);
	}

	void CommandListVK::PipelineBufferBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineBufferBarrierDesc* pBufferBarriers, uint32 bufferBarrierCount)
	{
		VALIDATE(pBufferBarriers		!= nullptr);
		VALIDATE(bufferBarrierCount	<= MAX_BUFFER_BARRIERS);

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
		vkCmdPipelineBarrier(m_CmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, bufferBarrierCount, m_BufferBarriers, 0, nullptr);
	}

	void CommandListVK::PipelineMemoryBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineMemoryBarrierDesc* pMemoryBarriers, uint32 bufferMemoryCount)
	{
		VALIDATE(pMemoryBarriers != nullptr);
		VALIDATE(bufferMemoryCount < MAX_MEMORY_BARRIERS);

		for (uint32 i = 0; i < bufferMemoryCount; i++)
		{
			const PipelineMemoryBarrierDesc* pBarrier = &pMemoryBarriers[i];

			m_MemoryBarriers[i].sType				= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			m_MemoryBarriers[i].pNext				= nullptr;
			m_MemoryBarriers[i].srcAccessMask		= ConvertMemoryAccessFlags(pBarrier->SrcMemoryAccessFlags);
			m_MemoryBarriers[i].dstAccessMask		= ConvertMemoryAccessFlags(pBarrier->DstMemoryAccessFlags);
		}

		VkPipelineStageFlags sourceStage		= ConvertPipelineStageMask(srcStage);
		VkPipelineStageFlags destinationStage	= ConvertPipelineStageMask(dstStage);
		vkCmdPipelineBarrier(m_CmdBuffer, sourceStage, destinationStage, 0, bufferMemoryCount, m_MemoryBarriers, 0, nullptr, 0, nullptr);
	}

	void CommandListVK::GenerateMips(Texture* pTexture, ETextureState stateBefore, ETextureState stateAfter, bool linearFiltering)
	{
		VALIDATE(pTexture != nullptr);

		// Start by flushing barriers
		FlushDeferredBarriers();

		TextureVK*		pVkTexture		= reinterpret_cast<TextureVK*>(pTexture);
		TextureDesc		desc			= pVkTexture->GetDesc();
		const uint32	mipLevelCount	= desc.Miplevels;

		if (mipLevelCount < 2)
		{
			LOG_WARNING("[CommandListVK::GenerateMips]: pTexture only has 1 miplevel allocated, no other mips will be generated");
			return;
		}

		constexpr uint32 REQUIRED_FLAGS = (TEXTURE_FLAG_COPY_SRC | TEXTURE_FLAG_COPY_DST);
		if ((desc.Flags & REQUIRED_FLAGS) != REQUIRED_FLAGS)
		{
			LOG_ERROR("[CommandListVK::GenerateMips]: pTexture were not created with TEXTURE_FLAG_COPY_SRC and TEXTURE_FLAG_COPY_DST flags");
			DEBUGBREAK();
			return;
		}

		// TODO: Fix for devices that do NOT support linear filtering
		if (linearFiltering)
		{
			VkFormat			formatVk			= ConvertFormat(desc.Format);
			VkFormatProperties	formatProperties	= m_pDevice->GetFormatProperties(formatVk);
			if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
			{
				LOG_ERROR("[CommandListVK]: Device does not support generating miplevels at the moment");
				return;
			}
		}

		PipelineTextureBarrierDesc textureBarrier = { };
		textureBarrier.pTexture		= pTexture;
		textureBarrier.TextureFlags	= desc.Flags;
		textureBarrier.QueueAfter	= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		textureBarrier.QueueBefore	= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;

		if (stateBefore != ETextureState::TEXTURE_STATE_COPY_DST)
		{
			textureBarrier.Miplevel				= 0;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.MiplevelCount		= desc.Miplevels;
			textureBarrier.ArrayCount			= desc.ArrayCount;
			textureBarrier.SrcMemoryAccessFlags = 0;
			textureBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			textureBarrier.StateBefore			= stateBefore;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_COPY_DST;
			PipelineTextureBarriers(
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, 
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, 
				&textureBarrier, 1);
		}

		VkFilter	filterType			= linearFiltering ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		VkImage		imageVk				= pVkTexture->GetImage();
		VkExtent2D	destinationExtent	= {};
		VkExtent2D	sourceExtent		= { desc.Width, desc.Height };

		for (uint32_t i = 1; i < mipLevelCount; i++)
		{
			destinationExtent = { std::max(sourceExtent.width / 2U, 1u), std::max(sourceExtent.height / 2U, 1U) };

			textureBarrier.Miplevel				= i - 1;
			textureBarrier.MiplevelCount		= 1;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.ArrayCount			= desc.ArrayCount;
			textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_COPY_DST;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_COPY_SRC;
			textureBarrier.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			textureBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
			PipelineTextureBarriers(
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, 
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, 
				&textureBarrier, 1);

			VkImageBlit blit = {};
			blit.srcOffsets[0]					= { 0, 0, 0 };
			blit.srcOffsets[1]					= { int32_t(sourceExtent.width), int32_t(sourceExtent.height), int32_t(1) };
			blit.srcSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel		= i - 1;
			blit.srcSubresource.baseArrayLayer	= 0;
			blit.srcSubresource.layerCount		= desc.ArrayCount;
			blit.dstOffsets[0]					= { 0, 0, 0 };
			blit.dstOffsets[1]					= { int32_t(destinationExtent.width), int32_t(destinationExtent.height), int32_t(1) };
			blit.dstSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel		= i;
			blit.dstSubresource.baseArrayLayer	= 0;
			blit.dstSubresource.layerCount		= desc.ArrayCount;

			vkCmdBlitImage(
				m_CmdBuffer, 
				imageVk, 
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
				imageVk, 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
				1, &blit, 
				filterType);

			sourceExtent = destinationExtent;
		}

		textureBarrier.Miplevel = mipLevelCount - 1;
		PipelineTextureBarriers(
			FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, 
			FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, 
			&textureBarrier, 1);

		if (stateAfter != ETextureState::TEXTURE_STATE_COPY_SRC)
		{
			textureBarrier.Miplevel			= 0;
			textureBarrier.MiplevelCount	= desc.Miplevels;
			textureBarrier.ArrayCount		= desc.ArrayCount;
			textureBarrier.StateBefore		= ETextureState::TEXTURE_STATE_COPY_SRC;
			textureBarrier.StateAfter		= stateAfter;
			PipelineTextureBarriers(
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, 
				FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM, 
				&textureBarrier, 1);
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

		vkCmdSetViewport(m_CmdBuffer, firstViewport, viewportCount, m_Viewports);
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

		vkCmdSetScissor(m_CmdBuffer, firstScissor, scissorCount, m_ScissorRects);
	}

	void CommandListVK::SetStencilTestReference(EStencilFace face, uint32 reference)
	{
		VkStencilFaceFlags	faceFlagsVK = ConvertStencilFace(face);

		vkCmdSetStencilReference(m_CmdBuffer, faceFlagsVK, reference);
	}

	void CommandListVK::SetConstantRange(const PipelineLayout* pPipelineLayout, uint32 shaderStageMask, const void* pConstants, uint32 size, uint32 offset)
	{
		const PipelineLayoutVK* pVkPipelineLayout = reinterpret_cast<const PipelineLayoutVK*>(pPipelineLayout);
		uint32 shaderStageMaskVk = ConvertShaderStageMask(shaderStageMask);

		vkCmdPushConstants(m_CmdBuffer, pVkPipelineLayout->GetPipelineLayout(), shaderStageMaskVk, offset, size, pConstants);
	}

	void CommandListVK::BindIndexBuffer(const Buffer* pIndexBuffer, uint64 offset, EIndexType indexType)
	{
		const BufferVK* pIndexBufferVK = reinterpret_cast<const BufferVK*>(pIndexBuffer);
		vkCmdBindIndexBuffer(m_CmdBuffer, pIndexBufferVK->GetBuffer(), offset, ConvertIndexType(indexType));
	}

	void CommandListVK::BindVertexBuffers(const Buffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount)
	{
		for (uint32 i = 0; i < vertexBufferCount; i++)
		{
			const BufferVK* pVertexBufferVK = reinterpret_cast<const BufferVK*>(ppVertexBuffers[i]);

			m_VertexBuffers[i]          = pVertexBufferVK->GetBuffer();
			m_VertexBufferOffsets[i]    = VkDeviceSize(pOffsets[i]);
		}

		vkCmdBindVertexBuffers(m_CmdBuffer, firstBuffer, vertexBufferCount, m_VertexBuffers, m_VertexBufferOffsets);
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
		vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineVk->GetPipeline());
	}

	void CommandListVK::BindComputePipeline(const PipelineState* pPipeline)
	{
		CHECK_COMPUTE(m_Allocator);
		VALIDATE(pPipeline->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE);

		const ComputePipelineStateVK* pPipelineVk = reinterpret_cast<const ComputePipelineStateVK*>(pPipeline);
		vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineVk->GetPipeline());
	}

	void CommandListVK::BindRayTracingPipeline(PipelineState* pPipeline)
	{
		CHECK_COMPUTE(m_Allocator);
		VALIDATE(pPipeline->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING);

		m_CurrentRayTracingPipeline = reinterpret_cast<RayTracingPipelineStateVK*>(pPipeline);
		m_CurrentRayTracingPipeline->AddRef();

		vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_CurrentRayTracingPipeline->GetPipeline());
	}

	void CommandListVK::TraceRays(const SBT* pSBT, uint32 width, uint32 height, uint32 depth)
	{
		CHECK_COMPUTE(m_Allocator);
		VALIDATE(m_pDevice->vkCmdTraceRaysKHR);

		const SBTVK* pSBTVK = reinterpret_cast<const SBTVK*>(pSBT);

		const VkStridedBufferRegionKHR* pRaygen		= pSBTVK->GetRaygenBufferRegion();
		const VkStridedBufferRegionKHR* pMiss		= pSBTVK->GetMissBufferRegion();
		const VkStridedBufferRegionKHR* pHit		= pSBTVK->GetHitBufferRegion();
		const VkStridedBufferRegionKHR* pCallable	= pSBTVK->GetCallableBufferRegion();

		// Start by flushing barriers
		FlushDeferredBarriers();

		m_pDevice->vkCmdTraceRaysKHR(m_CmdBuffer, pRaygen, pMiss, pHit, pCallable, width, height, depth);
		m_CurrentRayTracingPipeline.Reset();
	}

	void CommandListVK::Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ)
	{
		CHECK_COMPUTE(m_Allocator);

		// Start by flushing barriers
		FlushDeferredBarriers();

		vkCmdDispatch(m_CmdBuffer, workGroupCountX, workGroupCountY, workGroupCountZ);
	}

	void CommandListVK::DispatchMesh(uint32 taskCount, uint32 firstTask)
	{
		CHECK_GRAPHICS(m_Allocator);

		// Start by flushing barriers
		FlushDeferredBarriers();

		VALIDATE(m_pDevice->vkCmdDrawMeshTasksNV != nullptr);
		m_pDevice->vkCmdDrawMeshTasksNV(m_CmdBuffer, taskCount, firstTask);
	}

	void CommandListVK::DispatchMeshIndirect(const Buffer* pDrawBuffer, uint32 offset, uint32 drawCount, uint32 stride)
	{
		CHECK_GRAPHICS(m_Allocator);

		// Start by flushing barriers
		FlushDeferredBarriers();

		VALIDATE(m_pDevice->vkCmdDrawMeshTasksIndirectNV != nullptr);

		const BufferVK* pDrawBufferVk = reinterpret_cast<const BufferVK*>(pDrawBuffer);
		m_pDevice->vkCmdDrawMeshTasksIndirectNV(m_CmdBuffer, pDrawBufferVk->GetBuffer(), offset, drawCount, stride);
	}

	void CommandListVK::DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
		CHECK_GRAPHICS(m_Allocator);

		// Start by flushing barriers
		FlushDeferredBarriers();

		vkCmdDraw(m_CmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandListVK::DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)
	{
		CHECK_GRAPHICS(m_Allocator);

		// Start by flushing barriers
		FlushDeferredBarriers();

		vkCmdDrawIndexed(m_CmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandListVK::DrawIndexedIndirect(const Buffer* pDrawBuffer, uint32 offset, uint32 drawCount, uint32 stride)
	{
		CHECK_GRAPHICS(m_Allocator);

		// Start by flushing barriers
		FlushDeferredBarriers();

		const BufferVK* pDrawBufferVk = reinterpret_cast<const BufferVK*>(pDrawBuffer);
		vkCmdDrawIndexedIndirect(m_CmdBuffer, pDrawBufferVk->GetBuffer(), offset, drawCount, stride);
	}

	void CommandListVK::BeginQuery(QueryHeap* pQueryHeap, uint32 queryIndex)
	{
		QueryHeapVK*		pQueryHeapVk	= reinterpret_cast<QueryHeapVK*>(pQueryHeap);
		VkQueryControlFlags controlFlagsVk	= 0; //VK_QUERY_CONTROL_PRECISE_BIT;
		vkCmdBeginQuery(m_CmdBuffer, pQueryHeapVk->GetQueryPool(), queryIndex, controlFlagsVk);
	}

	void CommandListVK::Timestamp(QueryHeap* pQueryHeap, uint32 queryIndex, FPipelineStageFlags pipelineStageFlag)
	{
		QueryHeapVK*			pQueryHeapVk		= reinterpret_cast<QueryHeapVK*>(pQueryHeap);
		VkPipelineStageFlagBits	pipelineStageFlagVk	= ConvertPipelineStage(pipelineStageFlag);
		vkCmdWriteTimestamp(m_CmdBuffer, pipelineStageFlagVk, pQueryHeapVk->GetQueryPool(), queryIndex);
	}

	void CommandListVK::EndQuery(QueryHeap* pQueryHeap, uint32 queryIndex)
	{
		QueryHeapVK* pQueryHeapVk = reinterpret_cast<QueryHeapVK*>(pQueryHeap);
		vkCmdEndQuery(m_CmdBuffer, pQueryHeapVk->GetQueryPool(), queryIndex);
	}

	void CommandListVK::ResetQuery(QueryHeap* pQueryHeap, uint32 firstQuery, uint32 queryCount)
	{
		QueryHeapVK* pQueryHeapVk = reinterpret_cast<QueryHeapVK*>(pQueryHeap);
		vkCmdResetQueryPool(m_CmdBuffer, pQueryHeapVk->GetQueryPool(), firstQuery, queryCount);
	}

	void CommandListVK::SetLineWidth(float32 lineWidth)
	{
		vkCmdSetLineWidth(m_CmdBuffer, lineWidth);
	}

	void CommandListVK::ExecuteSecondary(const CommandList* pSecondary)
	{
		VALIDATE(m_Desc.CommandListType == ECommandListType::COMMAND_LIST_TYPE_PRIMARY);
		const CommandListVK* pVkSecondary = reinterpret_cast<const CommandListVK*>(pSecondary);

#ifndef LAMBDA_DISABLE_ASSERTS
		CommandListDesc	desc = pVkSecondary->GetDesc();
		VALIDATE(desc.CommandListType == ECommandListType::COMMAND_LIST_TYPE_SECONDARY);
#endif

		vkCmdExecuteCommands(m_CmdBuffer, 1, &pVkSecondary->m_CmdBuffer);
	}

	void CommandListVK::DeferDestruction(DeviceChild* pResource)
	{
		pResource->AddRef();
		m_ResourcesToDestroy.EmplaceBack(TSharedRef<DeviceChild>(pResource));
	}

	void CommandListVK::FlushDeferredBarriers()
	{
		if (!m_DeferredBarriers.IsEmpty())
		{
			for (DeferredImageBarrier& barrier : m_DeferredBarriers)
			{
				vkCmdPipelineBarrier(m_CmdBuffer, barrier.SrcStages, barrier.DestStages, 0, 0, nullptr, 0, nullptr, barrier.Barriers.GetSize(), barrier.Barriers.GetData());
			}

			m_DeferredBarriers.Clear();
		}
	}

	void CommandListVK::FlushDeferredResources()
	{
		m_ResourcesToDestroy.Clear();
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
		vkCmdBindDescriptorSets(m_CmdBuffer, bindPoint, pVkPipelineLayout->GetPipelineLayout(), setIndex, 1, &descriptorSet, 0, nullptr);
	}
}
