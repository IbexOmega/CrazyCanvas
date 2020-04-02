#include "Log/Log.h"

#include <algorithm>

#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	CommandListVK::CommandListVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_Desc(),
		m_ImageBarriers()
	{
	}

	CommandListVK::~CommandListVK()
	{
		SAFERELEASE(m_pAllocator);
		m_CommandList = VK_NULL_HANDLE;
	}

	bool CommandListVK::Init(ICommandAllocator* pAllocator, const CommandListDesc& desc)
	{
		CommandAllocatorVK* pVkCommandAllocator = (CommandAllocatorVK*)pAllocator;
		pVkCommandAllocator->AddRef();

		VkCommandBufferLevel level;
		if (desc.CommandListType == ECommandListType::COMMANDLIST_PRIMARY)
		{
			level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		}
		else
		{
			level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		}

		m_CommandList = pVkCommandAllocator->AllocateCommandBuffer(level);
		if (m_CommandList == VK_NULL_HANDLE)
		{
			return false;
		}

		m_Desc			= desc;
		m_pAllocator	= pVkCommandAllocator;
		return true;
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
		if (m_Desc.Flags & ECommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT)
		{
			beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		}
		
		VkCommandBufferInheritanceInfo inheritanceInfo = { };
		if (!pBeginDesc)
		{
			beginInfo.pInheritanceInfo	= nullptr;
		}
		else
		{
			inheritanceInfo.sType					= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritanceInfo.pNext					= nullptr;
			//inheritanceInfo.framebuffer			= ;
			//inheritanceInfo.renderPass			= ;
			//inheritanceInfo.subpass				= ;
			inheritanceInfo.occlusionQueryEnable	= VK_FALSE;
			inheritanceInfo.pipelineStatistics		= 0;
			inheritanceInfo.queryFlags				= 0;
			
			beginInfo.pInheritanceInfo = &inheritanceInfo;
		}

		VkResult result = vkBeginCommandBuffer(m_CommandList, &beginInfo);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[CommandListVK]: Begin CommandBuffer Failed", result);
		}
	}

	void CommandListVK::End()
	{
		VkResult result = vkEndCommandBuffer(m_CommandList); 
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR("[CommandListVK]: End CommandBuffer Failed", result);
		}
	}

	void CommandListVK::BeginRenderPass(const IRenderPass* pRenderPass, const IFrameBuffer* pFrameBuffer, uint32 width, uint32 height, uint32 flags)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.pNext = nullptr;
		//renderPassInfo.renderPass			= pRenderPass->getRenderPass();
		//renderPassInfo.framebuffer		= pFrameBuffer->getFrameBuffer();
		renderPassInfo.renderArea.offset	= { 0, 0 };
		renderPassInfo.renderArea.extent	= { width, height };
		//renderPassInfo.pClearValues			= pClearVales;
		//renderPassInfo.clearValueCount		= clearValueCount;

		VkSubpassContents subpassContent;
		if (flags & ERenderPassBeginFlags::RENDER_PASS_BEGIN_FLAG_INLINE)
		{
			subpassContent = VK_SUBPASS_CONTENTS_INLINE;
		}
		else if (flags & ERenderPassBeginFlags::RENDER_PASS_BEGIN_FLAG_EXECUTE_SECONDARY)
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
		TextureVK* pVkDst		= reinterpret_cast<TextureVK*>(pDst);

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

	void CommandListVK::PipelineTextureBarriers(EPipelineStage srcStage, EPipelineStage dstStage, const PipelineTextureBarrier* pTextureBarriers, uint32 textureBarrierCount)
	{
		ASSERT(textureBarrierCount < MAX_IMAGE_BARRIERS);

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
			m_ImageBarriers[i].image							= pVkTexture->GetImage();
			m_ImageBarriers[i].srcQueueFamilyIndex				= m_pDevice->GetQueueFamilyIndexFromQueueType(barrier.QueueBefore);
			m_ImageBarriers[i].dstQueueFamilyIndex				= m_pDevice->GetQueueFamilyIndexFromQueueType(barrier.QueueAfter);
			m_ImageBarriers[i].oldLayout						= oldLayout;
			m_ImageBarriers[i].newLayout						= newLayout;
			m_ImageBarriers[i].subresourceRange.baseMipLevel	= barrier.Miplevel;
			m_ImageBarriers[i].subresourceRange.levelCount		= barrier.MiplevelCount;
			m_ImageBarriers[i].subresourceRange.baseArrayLayer	= barrier.ArrayIndex;
			m_ImageBarriers[i].subresourceRange.layerCount		= barrier.ArrayCount;
			m_ImageBarriers[i].srcAccessMask					= 0;
			m_ImageBarriers[i].dstAccessMask					= 0;

			if (barrier.TextureFlags == ETextureFlags::TEXTURE_FLAG_DEPTH_STENCIL)
			{
				m_ImageBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else
			{
				m_ImageBarriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
		}

		VkPipelineStageFlags sourceStage		= ConvertPipelineStage(srcStage);
		VkPipelineStageFlags destinationStage	= ConvertPipelineStage(dstStage);
		vkCmdPipelineBarrier(m_CommandList, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, textureBarrierCount, m_ImageBarriers);
	}

	void CommandListVK::GenerateMiplevels(ITexture* pTexture, ETextureState stateBefore, ETextureState stateAfter)
	{
		ASSERT(pTexture != nullptr);

		TextureVK*		pVkTexture		= reinterpret_cast<TextureVK*>(pTexture);
		TextureDesc		desc			= pVkTexture->GetDesc();
		const uint32	miplevelCount	= desc.Miplevels;
		
		//TODO: Fix for devices that do not support linear filtering
		VkFormat			formatVk			= ConvertFormat(desc.Format);
		VkFormatProperties	formatProperties	= m_pDevice->GetFormatProperties(formatVk);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			LOG_ERROR("[CommandListVK]: Device does not support generating miplevels at the moment");
			return;
		}

		PipelineTextureBarrier textureBarrier = { };
		textureBarrier.pTexture			= pTexture;
		textureBarrier.TextureFlags		= desc.Flags;
		textureBarrier.StateBefore		= stateBefore;
		textureBarrier.StateAfter		= ETextureState::TEXTURE_STATE_COPY_DST;
		textureBarrier.QueueAfter		= ECommandQueueType::COMMAND_QUEUE_IGNORE;
		textureBarrier.QueueBefore		= ECommandQueueType::COMMAND_QUEUE_IGNORE;
		textureBarrier.Miplevel			= 0;
		textureBarrier.MiplevelCount	= desc.Miplevels;
		textureBarrier.ArrayIndex		= 0;
		textureBarrier.ArrayCount		= desc.ArrayCount;

		PipelineTextureBarriers(EPipelineStage::PIPELINE_STAGE_TOP, EPipelineStage::PIPELINE_STAGE_COPY, &textureBarrier, 1);

		VkImage		imageVk				= pVkTexture->GetImage();
		VkExtent2D	destinationExtent	= {};
		VkExtent2D	sourceExtent		= { desc.Width, desc.Height };
		for (uint32_t i = 1; i < miplevelCount; i++)
		{
			destinationExtent = { std::max(sourceExtent.width / 2U, 1u), std::max(sourceExtent.height / 2U, 1U) };

			textureBarrier.Miplevel			= i - 1;
			textureBarrier.MiplevelCount	= 1;
			textureBarrier.ArrayCount		= 1;
			textureBarrier.StateBefore		= ETextureState::TEXTURE_STATE_COPY_DST;
			textureBarrier.StateAfter		= ETextureState::TEXTURE_STATE_COPY_SRC;
			PipelineTextureBarriers(EPipelineStage::PIPELINE_STAGE_COPY, EPipelineStage::PIPELINE_STAGE_COPY, &textureBarrier, 1);

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
		PipelineTextureBarriers(EPipelineStage::PIPELINE_STAGE_TOP, EPipelineStage::PIPELINE_STAGE_COPY, &textureBarrier, 1);

		textureBarrier.Miplevel			= 0;
		textureBarrier.MiplevelCount	= desc.Miplevels;
		textureBarrier.ArrayCount		= desc.ArrayCount;
		textureBarrier.StateBefore		= ETextureState::TEXTURE_STATE_COPY_SRC;
		textureBarrier.StateAfter		= stateAfter;
		PipelineTextureBarriers(EPipelineStage::PIPELINE_STAGE_COPY, EPipelineStage::PIPELINE_STAGE_BOTTOM, &textureBarrier, 1);
	}

	void CommandListVK::SetViewports(const Viewport* pViewports)
	{
	}

	void CommandListVK::SetScissorRects(const ScissorRect* pScissorRects)
	{
	}

	void CommandListVK::SetConstantGraphics()
	{
	}

	void CommandListVK::SetConstantCompute()
	{
	}

	void CommandListVK::BindIndexBuffer(const IBuffer* pIndexBuffer)
	{
	}

	void CommandListVK::BindVertexBuffers(const IBuffer* const* ppVertexBuffers, const uint32* pOffsets, uint32 vertexBufferCount)
	{
	}

	void CommandListVK::BindDescriptorSet(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout)
	{
	}

	void CommandListVK::BindGraphicsPipeline(const IPipelineState* pPipeline)
	{
	}

	void CommandListVK::BindComputePipeline(const IPipelineState* pPipeline)
	{
	}

	void CommandListVK::BindRayTracingPipeline(const IPipelineState* pPipeline)
	{
	}

	void CommandListVK::TraceRays(uint32 width, uint32 height, uint32 raygenOffset)
	{
	}

	void CommandListVK::Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ)
	{
	}

	void CommandListVK::DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
	}

	void CommandListVK::DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)
	{
	}

	void CommandListVK::ExecuteSecondary(const ICommandList* pSecondary)
	{
		const CommandListVK*	pVkSecondary	= reinterpret_cast<const CommandListVK*>(pSecondary);
		CommandListDesc			desc			= pVkSecondary->GetDesc();

		ASSERT(desc.CommandListType == ECommandListType::COMMANDLIST_SECONDARY);

		vkCmdExecuteCommands(m_CommandList, 1, &pVkSecondary->m_CommandList);
	}
	
	ICommandAllocator* CommandListVK::GetAllocator() const
	{
		m_pAllocator->AddRef();
		return m_pAllocator;
	}
}