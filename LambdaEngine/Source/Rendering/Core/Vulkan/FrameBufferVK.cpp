#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FrameBufferVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"

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
        for (uint32 i = 0; i < m_Desc.RenderTargetCount; i++)
        {
            SAFERELEASE(m_ppRenderTargets[i]);
        }

        SAFERELEASE(m_pDepthStencil);
    }

    bool FrameBufferVK::Init(const FrameBufferDesc& desc)
    {
        VkFramebufferCreateInfo createInfo = { };
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.pNext = nullptr;
        //createInfo.flags = ;
        createInfo.width = desc.Width;
        return true;
    }

    void FrameBufferVK::SetName(const char* pName)
    {
        m_pDevice->SetVulkanObjectName(pName, (uint64)m_FrameBuffer, VK_OBJECT_TYPE_FRAMEBUFFER);
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
            LOG_ERROR("[FrameBufferVK]: Trying to get depthstencil from a framebuffer without depthstencil");
            return nullptr;
        }
    }
}
