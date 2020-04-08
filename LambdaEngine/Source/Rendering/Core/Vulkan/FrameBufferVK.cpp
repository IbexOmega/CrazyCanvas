#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FrameBufferVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/RenderPassVK.h"

#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
    FrameBufferVK::FrameBufferVK(const GraphicsDeviceVK* pDevice)
        : TDeviceChild(pDevice),
        m_ppRenderTargets(),
        m_Desc()
    {
        memset(m_ppRenderTargets, 0, sizeof(m_ppRenderTargets));
    }

    FrameBufferVK::~FrameBufferVK()
    {
        if (m_FrameBuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(m_pDevice->Device, m_FrameBuffer, nullptr);
            m_FrameBuffer = VK_NULL_HANDLE;
        }

        for (uint32 i = 0; i < m_Desc.RenderTargetCount; i++)
        {
            SAFERELEASE(m_ppRenderTargets[i]);
        }

        SAFERELEASE(m_pDepthStencil);
    }

    bool FrameBufferVK::Init(const IRenderPass* pRenderPass, const FrameBufferDesc& desc)
    {
        uint32      attachmentCount = 0;
        VkImageView attachments[MAX_RENDERTARGETS + 1];
        for (uint32 i = 0; i < desc.RenderTargetCount; i++)
        {
            TextureViewVK* pTextureViewVk = reinterpret_cast<TextureViewVK*>(desc.ppRenderTargets[i]);
            attachments[i] = pTextureViewVk->GetImageView();

            pTextureViewVk->AddRef();
            m_ppRenderTargets[i] = pTextureViewVk;

            attachmentCount++;
        }

        if (desc.pDepthStencil)
        {
            TextureViewVK* pTextureViewVk = reinterpret_cast<TextureViewVK*>(desc.pDepthStencil);
            attachments[attachmentCount] = pTextureViewVk->GetImageView();

            pTextureViewVk->AddRef();
            m_pDepthStencil = pTextureViewVk;

            attachmentCount++;
        }

        VkFramebufferCreateInfo createInfo  = { };
        createInfo.sType            = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.pNext            = nullptr;
        createInfo.flags            = 0; //TODO: Support imageless framebuffers
        createInfo.width            = desc.Width;
        createInfo.height           = desc.Height;
        createInfo.layers           = 1;
        createInfo.attachmentCount  = attachmentCount;
        createInfo.pAttachments     = attachments;
        
        const RenderPassVK* pRenderPassVk = reinterpret_cast<const RenderPassVK*>(pRenderPass);
        createInfo.renderPass = pRenderPassVk->GetRenderPass();

        VkResult result = vkCreateFramebuffer(m_pDevice->Device, &createInfo, nullptr, &m_FrameBuffer);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[FrameBufferVK]: Failed to create framebuffer");
            return false;
        }
        else
        {
            m_Desc = desc;
            SetName(m_Desc.pName);

            D_LOG_MESSAGE("[FrameBufferVK]: Created framebuffer");
            return true;
        }
    }

    void FrameBufferVK::SetName(const char* pName)
    {
        if (pName)
        {
            TDeviceChild::SetName(pName);
            m_pDevice->SetVulkanObjectName(pName, (uint64)m_FrameBuffer, VK_OBJECT_TYPE_FRAMEBUFFER);

            m_Desc.pName = m_DebugName;
        }
    }

    ITextureView* FrameBufferVK::GetRenderTarget(uint32 index)
    {
        ASSERT(index < m_Desc.RenderTargetCount);
        
        TextureViewVK* pRenderTargetVk = m_ppRenderTargets[index];
        pRenderTargetVk->AddRef();

        return pRenderTargetVk;
    }

    ITextureView* FrameBufferVK::GetDepthStencil()
    {
        if (m_pDepthStencil)
        {
            m_pDepthStencil->AddRef();
            return m_pDepthStencil;
        }
        else
        {
            LOG_ERROR("[FrameBufferVK]: Calling GetDepthStencil on a framebuffer without depthstencil");
            return nullptr;
        }
    }
}
