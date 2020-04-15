#include "Rendering/Core/Vulkan/RenderPassVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	RenderPassVK::RenderPassVK(const GraphicsDeviceVK* pDevice) :
		TDeviceChild(pDevice),
		m_RenderPass(VK_NULL_HANDLE)
	{
	}

	RenderPassVK::~RenderPassVK()
	{
		if (m_RenderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(m_pDevice->Device, m_RenderPass, nullptr);
			m_RenderPass = VK_NULL_HANDLE;
		}
	}

	bool RenderPassVK::Init(const RenderPassDesc& desc)
	{
		SubpassData				subpasses[MAX_SUBPASSES];
		VkSubpassDependency		subpassDependencies[MAX_SUBPASS_DEPENDENCIES];
		VkAttachmentDescription attachments[MAX_COLOR_ATTACHMENTS];

		CreateAttachmentDescriptions(desc, attachments);
		CreateSubpassDescriptions(desc, subpasses);
		CreateSubpassDependencies(desc, subpassDependencies);

		VkSubpassDescription subpassDescriptions[MAX_SUBPASSES];
		for (uint32 i = 0; i < desc.SubpassCount; i++)
		{
			subpassDescriptions[i] = subpasses[i].Subpass;
		}

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.pNext				= nullptr;
		renderPassCreateInfo.flags				= NULL;
		renderPassCreateInfo.attachmentCount	= desc.AttachmentCount;
		renderPassCreateInfo.pAttachments		= attachments;
		renderPassCreateInfo.subpassCount		= desc.SubpassCount;
		renderPassCreateInfo.pSubpasses			= subpassDescriptions;
		renderPassCreateInfo.dependencyCount	= desc.SubpassDependencyCount;
		renderPassCreateInfo.pDependencies		= subpassDependencies;

		if (vkCreateRenderPass(m_pDevice->Device, &renderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		{
			if (desc.pName)
			{
				LOG_ERROR("[RenderPassVK]: vkCreateRenderPass failed for \"%s\"", desc.pName);
			}
			else
			{
				LOG_ERROR("[RenderPassVK]: vkCreateRenderPass failed");
			}

			return false;
		}
		else
		{
			m_Desc = desc;
			SetName(desc.pName);

			if (desc.pName)
			{
				D_LOG_MESSAGE("[RenderPassVK]: Renderpass \"%s\" successfully initialized!", desc.pName);
			}
			else
			{
				D_LOG_MESSAGE("[RenderPassVK]: Renderpass successfully initialized!");
			}

			return true;
		}
	}

	void RenderPassVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_RenderPass, VK_OBJECT_TYPE_RENDER_PASS);

			m_Desc.pName = m_DebugName;
		}
	}

	void RenderPassVK::CreateAttachmentDescriptions(const RenderPassDesc& desc, VkAttachmentDescription* pResultAttachments)
	{
		ASSERT(desc.AttachmentCount <= MAX_COLOR_ATTACHMENTS);

		for (uint32 i = 0; i < desc.AttachmentCount; i++)
		{
			const RenderPassAttachmentDesc& attachment		= desc.pAttachments[i];
			VkAttachmentDescription&		vkAttachment	= pResultAttachments[i];

			vkAttachment = {};
			vkAttachment.flags				= NULL;
			vkAttachment.format				= ConvertFormat(attachment.Format);
			vkAttachment.samples			= ConvertSamples(attachment.SampleCount);
			vkAttachment.loadOp				= ConvertLoadOp(attachment.LoadOp);
			vkAttachment.storeOp			= ConvertStoreOp(attachment.StoreOp);
			vkAttachment.stencilLoadOp		= ConvertLoadOp(attachment.StencilLoadOp);
			vkAttachment.stencilStoreOp		= ConvertStoreOp(attachment.StencilStoreOp);
			vkAttachment.initialLayout		= ConvertTextureState(attachment.InitialState);
			vkAttachment.finalLayout		= ConvertTextureState(attachment.FinalState);
		}
	}

	void RenderPassVK::CreateSubpassDescriptions(const RenderPassDesc& desc, SubpassData* pResultSubpasses)
	{
		ASSERT(desc.SubpassCount <= MAX_SUBPASSES);

		for (uint32 i = 0; i < desc.SubpassCount; i++)
		{
			VkSubpassDescription&			vkSubpass	= pResultSubpasses[i].Subpass;
			const RenderPassSubpassDesc&	subpass		= desc.pSubpasses[i];

			ASSERT(subpass.InputAttachmentCount <= MAX_COLOR_ATTACHMENTS);
			ASSERT(subpass.RenderTargetCount	<= MAX_COLOR_ATTACHMENTS);
			
			vkSubpass = {};
			vkSubpass.flags					= 0;
			vkSubpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
			vkSubpass.inputAttachmentCount	= subpass.InputAttachmentCount;	
			
			//Input attachments
			for (uint32 attachment = 0; attachment < subpass.InputAttachmentCount; attachment++)
			{
				ETextureState			inputAttachmentState	= subpass.pInputAttachmentStates[attachment];
				VkAttachmentReference&	inputAttachment			= pResultSubpasses[i].InputAttachments[attachment];

				inputAttachment = {};
				if (inputAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN && inputAttachmentState != ETextureState::TEXTURE_STATE_DONT_CARE)
				{
					inputAttachment.attachment	= attachment;
					inputAttachment.layout		= ConvertTextureState(inputAttachmentState);
				}
				else
				{
					inputAttachment.attachment	= VK_ATTACHMENT_UNUSED;
				}
			}


			for (uint32 attachment = 0; attachment < subpass.RenderTargetCount; attachment++)
			{
				//Color Attachment
				{
					ETextureState			colorAttachmentState	= subpass.pRenderTargetStates[attachment];
					VkAttachmentReference&	colorAttachment			= pResultSubpasses[i].ColorAttachments[attachment];

					colorAttachment = {};
					if (colorAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN && colorAttachmentState != ETextureState::TEXTURE_STATE_DONT_CARE)
					{
						colorAttachment.attachment		= attachment;
						colorAttachment.layout			= ConvertTextureState(colorAttachmentState);
					}
					else
					{
						colorAttachment.attachment	= VK_ATTACHMENT_UNUSED;
					}
				}

				//Resolve Attachment
				if (subpass.pResolveAttachmentStates)
				{
					ETextureState			resolveAttachmentState	= subpass.pResolveAttachmentStates[attachment];
					VkAttachmentReference&	resolveAttachment		= pResultSubpasses[i].ResolveAttachments[attachment];

					resolveAttachment = {};
					if (resolveAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN && resolveAttachmentState != ETextureState::TEXTURE_STATE_DONT_CARE)
					{
						resolveAttachment.attachment = attachment;
						resolveAttachment.layout	 = ConvertTextureState(resolveAttachmentState);
					}
					else
					{
						resolveAttachment.attachment = VK_ATTACHMENT_UNUSED;
					}
				}
			}

			vkSubpass.inputAttachmentCount		= subpass.InputAttachmentCount;
			vkSubpass.pInputAttachments			= pResultSubpasses[i].InputAttachments;
			vkSubpass.colorAttachmentCount		= subpass.RenderTargetCount;
			vkSubpass.pColorAttachments			= pResultSubpasses[i].ColorAttachments;
			vkSubpass.pResolveAttachments		= pResultSubpasses[i].ResolveAttachments;
			vkSubpass.preserveAttachmentCount	= 0;
			vkSubpass.pResolveAttachments		= nullptr;
		}
	}

	void RenderPassVK::CreateSubpassDependencies(const RenderPassDesc& desc, VkSubpassDependency* pResultSubpassDependencies)
	{
		ASSERT(desc.SubpassDependencyCount <= MAX_SUBPASS_DEPENDENCIES);

		for (uint32 i = 0; i < desc.SubpassDependencyCount; i++)
		{
			VkSubpassDependency&					vkSubpassDependency = pResultSubpassDependencies[i];
			const RenderPassSubpassDependencyDesc&	subpassDependency	= desc.pSubpassDependencies[i];

			vkSubpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			vkSubpassDependency.srcSubpass		= subpassDependency.SrcSubpass != EXTERNAL_SUBPASS ? subpassDependency.SrcSubpass : VK_SUBPASS_EXTERNAL;
			vkSubpassDependency.dstSubpass		= subpassDependency.DstSubpass != EXTERNAL_SUBPASS ? subpassDependency.DstSubpass : VK_SUBPASS_EXTERNAL;
			vkSubpassDependency.srcStageMask	= ConvertPipelineStage(subpassDependency.SrcStageMask);
			vkSubpassDependency.dstStageMask	= ConvertPipelineStage(subpassDependency.DstStageMask);
			vkSubpassDependency.srcAccessMask	= ConvertMemoryAccessFlags(subpassDependency.SrcAccessMask);
			vkSubpassDependency.dstAccessMask	= ConvertMemoryAccessFlags(subpassDependency.DstAccessMask);
		}
	}
}