#include "Rendering/Core/Vulkan/RenderPassVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	RenderPassVK::RenderPassVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	RenderPassVK::~RenderPassVK()
	{
		m_pDevice->DestroyRenderPass(&m_RenderPass);
	}

	bool RenderPassVK::Init(const RenderPassDesc* pDesc)
	{
		SubpassData				subpasses[MAX_SUBPASSES];
		VkSubpassDependency		subpassDependencies[MAX_SUBPASS_DEPENDENCIES];
		VkAttachmentDescription attachments[MAX_COLOR_ATTACHMENTS + 1];

		CreateAttachmentDescriptions(pDesc, attachments);
		CreateSubpassDescriptions(pDesc, subpasses);
		CreateSubpassDependencies(pDesc, subpassDependencies);

		VkSubpassDescription subpassDescriptions[MAX_SUBPASSES];
		for (uint32 i = 0; i < pDesc->Subpasses.GetSize(); i++)
		{
			subpassDescriptions[i] = subpasses[i].Subpass;
		}

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.pNext				= nullptr;
		renderPassCreateInfo.flags				= NULL;
		renderPassCreateInfo.attachmentCount	= static_cast<uint32>(pDesc->Attachments.GetSize());
		renderPassCreateInfo.pAttachments		= attachments;
		renderPassCreateInfo.subpassCount		= static_cast<uint32>(pDesc->Subpasses.GetSize());
		renderPassCreateInfo.pSubpasses			= subpassDescriptions;
		renderPassCreateInfo.dependencyCount	= static_cast<uint32>(pDesc->SubpassDependencies.GetSize());
		renderPassCreateInfo.pDependencies		= subpassDependencies;

		VkResult result = vkCreateRenderPass(m_pDevice->Device, &renderPassCreateInfo, nullptr, &m_RenderPass);
		if (result != VK_SUCCESS)
		{
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "vkCreateRenderPass failed for \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "vkCreateRenderPass failed");
			}

			return false;
		}
		else
		{
			m_Desc = *pDesc;
			SetName(pDesc->DebugName);

			if (!pDesc->DebugName.empty())
			{
				LOG_DEBUG("Renderpass \"%s\" successfully initialized", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_DEBUG("Renderpass successfully initialized");
			}

			return true;
		}
	}

	void RenderPassVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_RenderPass), VK_OBJECT_TYPE_RENDER_PASS);
		m_Desc.DebugName = debugName;
	}

	void RenderPassVK::CreateAttachmentDescriptions(const RenderPassDesc* pDesc, VkAttachmentDescription* pResultAttachments)
	{
		//CreateSubpassDescriptions will discover if there are too many color attachments, we add 1 here to allow for one depth/stencil attachment as well
		VALIDATE(pDesc->Attachments.GetSize() <= MAX_COLOR_ATTACHMENTS + 1);

		for (uint32 i = 0; i < pDesc->Attachments.GetSize(); i++)
		{
			const RenderPassAttachmentDesc& attachment		= pDesc->Attachments[i];
			VkAttachmentDescription&		vkAttachment	= pResultAttachments[i];

			vkAttachment.flags				= 0;
			vkAttachment.format				= ConvertFormat(attachment.Format);
			vkAttachment.samples			= ConvertSampleCount(attachment.SampleCount);
			vkAttachment.loadOp				= ConvertLoadOp(attachment.LoadOp);
			vkAttachment.storeOp			= ConvertStoreOp(attachment.StoreOp);
			vkAttachment.stencilLoadOp		= ConvertLoadOp(attachment.StencilLoadOp);
			vkAttachment.stencilStoreOp		= ConvertStoreOp(attachment.StencilStoreOp);
			vkAttachment.initialLayout		= ConvertTextureState(attachment.InitialState);
			vkAttachment.finalLayout		= ConvertTextureState(attachment.FinalState);
		}
	}

	void RenderPassVK::CreateSubpassDescriptions(const RenderPassDesc* pDesc, SubpassData* pResultSubpasses)
	{
		VALIDATE(pDesc->Subpasses.GetSize() <= MAX_SUBPASSES);

		for (uint32 i = 0; i < pDesc->Subpasses.GetSize(); i++)
		{
			VkSubpassDescription&			vkSubpass	= pResultSubpasses[i].Subpass;
			const RenderPassSubpassDesc&	subpass		= pDesc->Subpasses[i];

			VALIDATE(subpass.InputAttachmentStates.GetSize()	<= MAX_COLOR_ATTACHMENTS);
			VALIDATE(subpass.RenderTargetStates.GetSize()		<= MAX_COLOR_ATTACHMENTS);

			vkSubpass.flags					= 0;
			vkSubpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
			vkSubpass.inputAttachmentCount	= static_cast<uint32>(subpass.InputAttachmentStates.GetSize());

			// Input attachments
			for (uint32 attachment = 0; attachment < static_cast<uint32>(subpass.InputAttachmentStates.GetSize()); attachment++)
			{
				ETextureState			inputAttachmentState	= subpass.InputAttachmentStates[attachment];
				VkAttachmentReference&	inputAttachment			= pResultSubpasses[i].InputAttachments[attachment];

				VALIDATE(inputAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN);

				inputAttachment = {};
				if (inputAttachmentState != ETextureState::TEXTURE_STATE_DONT_CARE)
				{
					inputAttachment.attachment	= attachment;
					inputAttachment.layout		= ConvertTextureState(inputAttachmentState);
				}
				else
				{
					inputAttachment.attachment	= VK_ATTACHMENT_UNUSED;
				}
			}

			for (uint32 attachment = 0; attachment < subpass.RenderTargetStates.GetSize(); attachment++)
			{
				// Color Attachment
				{
					ETextureState			colorAttachmentState	= subpass.RenderTargetStates[attachment];
					VkAttachmentReference&	colorAttachment			= pResultSubpasses[i].ColorAttachments[attachment];

					VALIDATE(colorAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN);

					colorAttachment = {};
					if (colorAttachmentState != ETextureState::TEXTURE_STATE_DONT_CARE)
					{
						colorAttachment.attachment		= attachment;
						colorAttachment.layout			= ConvertTextureState(colorAttachmentState);
					}
					else
					{
						colorAttachment.attachment	= VK_ATTACHMENT_UNUSED;
					}
				}

				// Resolve Attachment
				if (!subpass.ResolveAttachmentStates.IsEmpty())
				{
					ETextureState			resolveAttachmentState	= subpass.ResolveAttachmentStates[attachment];
					VkAttachmentReference&	resolveAttachment		= pResultSubpasses[i].ResolveAttachments[attachment];

					VALIDATE(resolveAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN);

					resolveAttachment = {};
					if (resolveAttachmentState != ETextureState::TEXTURE_STATE_DONT_CARE)
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

			// DepthStencil
			if (subpass.DepthStencilAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN && subpass.DepthStencilAttachmentState != ETextureState::TEXTURE_STATE_DONT_CARE)
			{
				pResultSubpasses[i].DepthStencil.attachment = static_cast<uint32>(subpass.RenderTargetStates.GetSize());
				pResultSubpasses[i].DepthStencil.layout		= ConvertTextureState(subpass.DepthStencilAttachmentState);

				vkSubpass.pDepthStencilAttachment	= &pResultSubpasses[i].DepthStencil;
			}
			else
			{
				vkSubpass.pDepthStencilAttachment = nullptr;
			}

			vkSubpass.inputAttachmentCount		= static_cast<uint32>(subpass.InputAttachmentStates.GetSize());
			vkSubpass.pInputAttachments			= pResultSubpasses[i].InputAttachments;
			vkSubpass.colorAttachmentCount		= static_cast<uint32>(subpass.RenderTargetStates.GetSize());
			vkSubpass.pColorAttachments			= pResultSubpasses[i].ColorAttachments;
			vkSubpass.pResolveAttachments		= pResultSubpasses[i].ResolveAttachments;
			vkSubpass.preserveAttachmentCount	= 0;
			vkSubpass.pResolveAttachments		= nullptr;
		}
	}

	void RenderPassVK::CreateSubpassDependencies(const RenderPassDesc* pDesc, VkSubpassDependency* pResultSubpassDependencies)
	{
		VALIDATE(pDesc->SubpassDependencies.GetSize() <= MAX_SUBPASS_DEPENDENCIES);

		for (uint32 i = 0; i < pDesc->SubpassDependencies.GetSize(); i++)
		{
			VkSubpassDependency&					vkSubpassDependency = pResultSubpassDependencies[i];
			const RenderPassSubpassDependencyDesc&	subpassDependency	= pDesc->SubpassDependencies[i];

			vkSubpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			vkSubpassDependency.srcSubpass		= (subpassDependency.SrcSubpass != EXTERNAL_SUBPASS) ? subpassDependency.SrcSubpass : VK_SUBPASS_EXTERNAL;
			vkSubpassDependency.dstSubpass		= (subpassDependency.DstSubpass != EXTERNAL_SUBPASS) ? subpassDependency.DstSubpass : VK_SUBPASS_EXTERNAL;
			vkSubpassDependency.srcStageMask	= ConvertPipelineStage(subpassDependency.SrcStageMask);
			vkSubpassDependency.dstStageMask	= ConvertPipelineStage(subpassDependency.DstStageMask);
			vkSubpassDependency.srcAccessMask	= ConvertMemoryAccessFlags(subpassDependency.SrcAccessMask);
			vkSubpassDependency.dstAccessMask	= ConvertMemoryAccessFlags(subpassDependency.DstAccessMask);
		}
	}
}
