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
		VkAttachmentDescription attachments[MAX_ATTACHMENTS];
		VkSubpassDescription subpasses[MAX_SUBPASSES];
		VkSubpassDependency subpassDependencies[MAX_SUBPASS_DEPENDENCIES];

		CreateAttachmentDescriptions(desc, attachments);
		CreateSubpassDescriptions(desc, subpasses);
		CreateSubpassDependencies(desc, subpassDependencies);

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.pNext				= nullptr;
		renderPassCreateInfo.flags				= NULL;
		renderPassCreateInfo.attachmentCount	= desc.AttachmentCount;
		renderPassCreateInfo.pAttachments		= attachments;
		renderPassCreateInfo.subpassCount		= desc.SubpassCount;
		renderPassCreateInfo.pSubpasses			= subpasses;
		renderPassCreateInfo.dependencyCount	= desc.SubpassDependencyCount;
		renderPassCreateInfo.pDependencies		= subpassDependencies;

		if (vkCreateRenderPass(m_pDevice->Device, &renderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
		{
			LOG_ERROR("[RenderPassVK]: vkCreateRenderPass failed for \"%s\"", desc.pName);
			return false;
		}

		SetName(desc.pName);

		D_LOG_MESSAGE("[RenderPassVK]: Renderpass \"%s\" successfully initialized!", desc.pName);

		return true;
	}

	void RenderPassVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_RenderPass, VK_OBJECT_TYPE_RENDER_PASS);
	}

	void RenderPassVK::CreateAttachmentDescriptions(const RenderPassDesc& desc, VkAttachmentDescription* pResultAttachments)
	{
		ASSERT(desc.AttachmentCount <= MAX_ATTACHMENTS);

		for (uint32 i = 0; i < desc.AttachmentCount; i++)
		{
			const RenderPassAttachmentDesc& attachment = desc.pAttachments[i];
			VkAttachmentDescription& vkAttachment = pResultAttachments[i];

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

	void RenderPassVK::CreateSubpassDescriptions(const RenderPassDesc& desc, VkSubpassDescription* pResultSubpasses)
	{
		ASSERT(desc.SubpassCount <= MAX_SUBPASSES);

		for (uint32 i = 0; i < desc.SubpassCount; i++)
		{
			const RenderPassSubpassDesc& subpass = desc.pSubpasses[i];
			VkSubpassDescription& vkSubpass = pResultSubpasses[i];

			vkSubpass = {};
			vkSubpass.flags = NULL;
			vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			vkSubpass.inputAttachmentCount = subpass.InputAttachmentCount;
			
			VkAttachmentReference inputAttachments[MAX_ATTACHMENTS];
			ASSERT(subpass.InputAttachmentCount <= MAX_ATTACHMENTS);

			for (uint32 a = 0; a < subpass.InputAttachmentCount; a++)
			{
				ETextureState attachmentState			= subpass.pInputAttachmentStates[a];
				VkAttachmentReference& inputAttachment	= inputAttachments[a];

				inputAttachment = {};

				if (attachmentState != ETextureState::TEXTURE_STATE_UNKNOWN)
				{
					inputAttachment.attachment		= a;
					inputAttachment.layout			= ConvertTextureState(attachmentState);
				}
				else
				{
					inputAttachment.attachment		= VK_ATTACHMENT_UNUSED;
				}
			}

			VkAttachmentReference colorAttachments[MAX_ATTACHMENTS];
			VkAttachmentReference resolveAttachments[MAX_ATTACHMENTS];
			ASSERT(subpass.ColorAttachmentCount <= MAX_ATTACHMENTS);

			for (uint32 a = 0; a < subpass.ColorAttachmentCount; a++)
			{
				//Color Attachment
				{
					ETextureState colorAttachmentState			= subpass.pColorAttachmentStates[a];
					VkAttachmentReference& colorAttachment		= colorAttachments[a];

					colorAttachment = {};

					if (colorAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN)
					{
						colorAttachment.attachment		= a;
						colorAttachment.layout			= ConvertTextureState(colorAttachmentState);
					}
					else
					{
						colorAttachment.attachment		= VK_ATTACHMENT_UNUSED;
					}
				}

				//Resolve Attachment
				{
					ETextureState resolveAttachmentState		= subpass.pResolveAttachmentStates[a];
					VkAttachmentReference& resolveAttachment	= resolveAttachments[a];

					resolveAttachment = {};

					if (resolveAttachmentState != ETextureState::TEXTURE_STATE_UNKNOWN)
					{
						resolveAttachment.attachment	= a;
						resolveAttachment.layout		= ConvertTextureState(resolveAttachmentState);
					}
					else
					{
						resolveAttachment.attachment	= VK_ATTACHMENT_UNUSED;
					}
				}
			}

			vkSubpass.inputAttachmentCount		= subpass.InputAttachmentCount;
			vkSubpass.pInputAttachments			= inputAttachments;
			vkSubpass.colorAttachmentCount		= subpass.ColorAttachmentCount;
			vkSubpass.pColorAttachments			= colorAttachments;
			vkSubpass.pResolveAttachments		= resolveAttachments;
			vkSubpass.preserveAttachmentCount	= 0;
			vkSubpass.pResolveAttachments		= nullptr;
		}
	}

	void RenderPassVK::CreateSubpassDependencies(const RenderPassDesc& desc, VkSubpassDependency* pResultSubpassDependencies)
	{
		ASSERT(desc.SubpassDependencyCount <= MAX_SUBPASS_DEPENDENCIES);

		for (uint32 i = 0; i < desc.SubpassDependencyCount; i++)
		{
			const RenderPassSubpassDependency& subpassDependency = desc.pSubpassDependencies[i];
			VkSubpassDependency& vkSubpassDependency = pResultSubpassDependencies[i];

			vkSubpassDependency.srcSubpass = subpassDependency.SrcSubpass != 0xFFFFFFFF ? subpassDependency.SrcSubpass : VK_SUBPASS_EXTERNAL;
			vkSubpassDependency.dstSubpass = subpassDependency.DstSubpass != 0xFFFFFFFF ? subpassDependency.DstSubpass : VK_SUBPASS_EXTERNAL;
			vkSubpassDependency.srcStageMask = ConvertPipelineStage(subpassDependency.SrcStageMask);
			vkSubpassDependency.dstStageMask = ConvertPipelineStage(subpassDependency.DstStageMask);
			vkSubpassDependency.srcAccessMask = ConvertAccessFlags(subpassDependency.SrcAccessMask);
			vkSubpassDependency.dstAccessMask = ConvertAccessFlags(subpassDependency.DstAccessMask);
		}
	}
}