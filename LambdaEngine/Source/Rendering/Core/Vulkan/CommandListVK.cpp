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
#include "Rendering/Core/Vulkan/FrameBufferVK.h"
#include "Rendering/Core/Vulkan/RenderPassVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	CommandListVK::CommandListVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
        m_ImageBarriers(),
        m_Viewports(),
        m_ScissorRects(),
        m_VertexBuffers(),
        m_VertexBufferOffsets(),
        m_Desc()
	{
	}

	CommandListVK::~CommandListVK()
	{
		if (m_pAllocator)
		{
			m_pAllocator->FreeCommandBuffer(m_CommandList);
			RELEASE(m_pAllocator);
		}

		m_CommandList = VK_NULL_HANDLE;
	}

	bool CommandListVK::Init(ICommandAllocator* pAllocator, const CommandListDesc& desc)
	{
		VkCommandBufferLevel level;
		if (desc.CommandListType == ECommandListType::COMMAND_LIST_PRIMARY)
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
			m_Desc = desc;
			SetName(desc.pName);
			
			pVkCommandAllocator->AddRef();
			m_pAllocator = pVkCommandAllocator;
			
			return true;
		}
	}

    void CommandListVK::SetName(const char* pName)
    {
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_CommandList, VK_OBJECT_TYPE_COMMAND_BUFFER);

			m_Desc.pName = m_DebugName;
		}
    }

	void CommandListVK::Reset()
	{
		vkResetCommandBuffer(m_CommandList, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}

	void CommandListVK::Begin(const SecondaryCommandListBeginDesc* pBeginDesc)
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
			const RenderPassVK*		pVkRenderPass	= reinterpret_cast<const RenderPassVK*>(pBeginDesc->pRenderPass);
			const FrameBufferVK*	pVkFrameBuffer	= reinterpret_cast<const FrameBufferVK*>(pBeginDesc->pFrameBuffer);

			ASSERT(pVkRenderPass != nullptr);
			ASSERT(pVkFrameBuffer != nullptr);

			inheritanceInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritanceInfo.pNext					= nullptr;
			inheritanceInfo.framebuffer				= pVkFrameBuffer->GetFrameBuffer();
			inheritanceInfo.renderPass				= pVkRenderPass->GetRenderPass();
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
		}
	}

	void CommandListVK::End()
	{
		VkResult result = vkEndCommandBuffer(m_CommandList); 
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[CommandListVK]: End CommandBuffer Failed");
		}
	}

	void CommandListVK::BeginRenderPass(const BeginRenderPassDesc* pBeginDesc)
	{
		ASSERT(pBeginDesc != nullptr);

		const RenderPassVK*		pVkRenderPass	= reinterpret_cast<const RenderPassVK*>(pBeginDesc->pRenderPass);
		const FrameBufferVK*	pVkFrameBuffer	= reinterpret_cast<const FrameBufferVK*>(pBeginDesc->pFrameBuffer);

		ASSERT(pVkRenderPass != nullptr);
		ASSERT(pVkFrameBuffer != nullptr);

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
		renderPassInfo.framebuffer			= pVkFrameBuffer->GetFrameBuffer();
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

	void CommandListVK::BuildTopLevelAccelerationStructure(IBottomLevelAccelerationStructure* pAccelerationStructure)
	{
	}

	void CommandListVK::BuildBottomLevelAccelerationStructure(IBottomLevelAccelerationStructure* pAccelerationStructure)
	{
	}

	void CommandListVK::CopyBuffer(const IBuffer* pSrc, uint64 srcOffset, IBuffer* pDst, uint64 dstOffset, uint64 sizeInBytes)
	{
		ASSERT(pSrc != nullptr);
		ASSERT(pDst != nullptr);

		const BufferVK* pVkSrc	= reinterpret_cast<const BufferVK*>(pSrc);
		BufferVK*		pVkDst	= reinterpret_cast<BufferVK*>(pDst);

		VkBufferCopy bufferCopy = {};
		bufferCopy.size			= sizeInBytes;
		bufferCopy.srcOffset	= srcOffset;
		bufferCopy.dstOffset	= dstOffset;

		vkCmdCopyBuffer(m_CommandList, pVkSrc->GetBuffer(), pVkDst->GetBuffer(), 1, &bufferCopy);
	}

	void CommandListVK::CopyTextureFromBuffer(const IBuffer* pSrc, ITexture* pDst, const CopyTextureFromBufferDesc& desc)
	{
		ASSERT(pSrc != nullptr);
		ASSERT(pDst != nullptr);

		const BufferVK* pVkSrc	= reinterpret_cast<const BufferVK*>(pSrc);
		TextureVK*		pVkDst	= reinterpret_cast<TextureVK*>(pDst);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferImageHeight				= desc.SrcHeight;
		copyRegion.bufferOffset						= desc.SrcOffset;
		copyRegion.bufferRowLength					= desc.SrcRowPitch;
		copyRegion.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT; //TODO: Other aspects
		copyRegion.imageSubresource.baseArrayLayer	= desc.ArrayIndex;
		copyRegion.imageSubresource.layerCount		= desc.ArrayCount;
		copyRegion.imageSubresource.mipLevel		= desc.Miplevel;
		copyRegion.imageExtent.depth				= desc.Depth;
		copyRegion.imageExtent.height				= desc.Height;
		copyRegion.imageExtent.width				= desc.Width;

		vkCmdCopyBufferToImage(m_CommandList, pVkSrc->GetBuffer(), pVkDst->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	}

	void CommandListVK::PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrier* pTextureBarriers, uint32 textureBarrierCount)
	{
		ASSERT(pTextureBarriers		!= nullptr);
		ASSERT(textureBarrierCount	< MAX_IMAGE_BARRIERS);

		TextureVK*		pVkTexture	= nullptr;
		VkImageLayout	oldLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout	newLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		for (uint32 i = 0; i < textureBarrierCount; i++)
		{
			const PipelineTextureBarrier& barrier = pTextureBarriers[i];

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

	void CommandListVK::PipelineBufferBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineBufferBarrier* pBufferBarriers, uint32 bufferBarrierCount)
	{
		ASSERT(pBufferBarriers		!= nullptr);
		ASSERT(bufferBarrierCount	< MAX_BUFFER_BARRIERS);

		BufferVK* pVkBuffer = nullptr;
		for (uint32 i = 0; i < bufferBarrierCount; i++)
		{
			const PipelineBufferBarrier& barrier = pBufferBarriers[i];
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
		vkCmdPipelineBarrier(m_CommandList, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, bufferBarrierCount, m_ImageBarriers);
	}

	void CommandListVK::GenerateMiplevels(ITexture* pTexture, ETextureState stateBefore, ETextureState stateAfter)
	{
		ASSERT(pTexture != nullptr);

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

		PipelineTextureBarrier textureBarrier = { };
		textureBarrier.pTexture				= pTexture;
		textureBarrier.TextureFlags			= desc.Flags;
		textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_NONE;
		textureBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_NONE;

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

	void CommandListVK::SetConstantGraphics()
	{
	}

	void CommandListVK::SetConstantCompute()
	{
	}

	void CommandListVK::BindIndexBuffer(const IBuffer* pIndexBuffer, uint64 offset)
	{
        const BufferVK* pIndexBufferVK = reinterpret_cast<const BufferVK*>(pIndexBuffer);
        vkCmdBindIndexBuffer(m_CommandList, pIndexBufferVK->GetBuffer(), offset, VK_INDEX_TYPE_UINT32);
	}

	void CommandListVK::BindVertexBuffers(const IBuffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount)
	{
        for (uint32 i = 0; i < vertexBufferCount; i++)
        {
            const BufferVK* pVertexBufferVK = reinterpret_cast<const BufferVK*>(ppVertexBuffers[i]);
            
            m_VertexBuffers[i]          = pVertexBufferVK->GetBuffer();
            m_VertexBufferOffsets[i]    = VkDeviceSize(pOffsets[i]);
        }
        
        vkCmdBindVertexBuffers(m_CommandList, firstBuffer, vertexBufferCount, m_VertexBuffers, m_VertexBufferOffsets);
	}

	void CommandListVK::BindDescriptorSet(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout)
	{
	}

	void CommandListVK::BindGraphicsPipeline(const IPipelineState* pPipeline)
	{
        ASSERT(pPipeline->GetType() == EPipelineStateType::GRAPHICS);
        
        const GraphicsPipelineStateVK* pPipelineVk = reinterpret_cast<const GraphicsPipelineStateVK*>(pPipeline);
        vkCmdBindPipeline(m_CommandList, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineVk->GetPipeline());
	}

	void CommandListVK::BindComputePipeline(const IPipelineState* pPipeline)
	{
        ASSERT(pPipeline->GetType() == EPipelineStateType::COMPUTE);
        
        const ComputePipelineStateVK* pPipelineVk = reinterpret_cast<const ComputePipelineStateVK*>(pPipeline);
        vkCmdBindPipeline(m_CommandList, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineVk->GetPipeline());
	}

	void CommandListVK::BindRayTracingPipeline(const IPipelineState* pPipeline)
	{
        ASSERT(pPipeline->GetType() == EPipelineStateType::RAY_TRACING);
        
        const RayTracingPipelineStateVK* pPipelineVk = reinterpret_cast<const RayTracingPipelineStateVK*>(pPipeline);
        m_pCurrentRayTracingPipeline = pPipelineVk;
        
        vkCmdBindPipeline(m_CommandList, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pPipelineVk->GetPipeline());
	}

	void CommandListVK::TraceRays(uint32 width, uint32 height, uint32 depth)
	{
#ifndef LAMBDA_PRODUCTION
        if (m_pDevice->vkCmdTraceRaysKHR)
        {
            //m_pDevice->vkCmdTraceRaysKHR(m_CommandList, , , , , width, height, depth);
        }
#else
        //m_pDevice->vkCmdTraceRaysKHR
#endif
	}

	void CommandListVK::Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ)
	{
        vkCmdDispatch(m_CommandList, workGroupCountX, workGroupCountY, workGroupCountZ);
	}

	void CommandListVK::DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
        vkCmdDraw(m_CommandList, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandListVK::DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)
	{
        vkCmdDrawIndexed(m_CommandList, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandListVK::ExecuteSecondary(const ICommandList* pSecondary)
	{
		const CommandListVK*	pVkSecondary	= reinterpret_cast<const CommandListVK*>(pSecondary);
		CommandListDesc			desc			= pVkSecondary->GetDesc();

		ASSERT(desc.CommandListType == ECommandListType::COMMAND_LIST_SECONDARY);

		vkCmdExecuteCommands(m_CommandList, 1, &pVkSecondary->m_CommandList);
	}
	
	ICommandAllocator* CommandListVK::GetAllocator() const
	{
		m_pAllocator->AddRef();
		return m_pAllocator;
	}
}
