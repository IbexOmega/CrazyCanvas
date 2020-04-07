#include "Rendering/Core/API/IFrameBuffer.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

#define MAX_RENDERTARGETS 8

namespace LambdaEngine
{
    class TextureViewVK;
    class GraphicsDeviceVK;

    class FrameBufferVK : public DeviceChildBase<GraphicsDeviceVK, IFrameBuffer>
    {
        using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IFrameBuffer>;
    
    public:
        FrameBufferVK(const GraphicsDeviceVK* pDevice);
        ~FrameBufferVK();

        bool Init(const IRenderPass* pRenderPass, const FrameBufferDesc& desc);

        FORCEINLINE VkFramebuffer GetFrameBuffer() const
        {
            return m_FrameBuffer;
        }

        //IDeviceChild interface
        virtual void SetName(const char* pName) override final;

        //IFrameBuffer interface
        virtual ITextureView* GetRenderTarget(uint32 index) override final;
        virtual ITextureView* GetDepthStencil()             override final;
        
        FORCEINLINE virtual FrameBufferDesc GetDesc() const override final
        {
            return m_Desc;
        }

    private:
        VkFramebuffer   m_FrameBuffer   = VK_NULL_HANDLE;
        TextureViewVK*  m_pDepthStencil = nullptr;

        TextureViewVK*  m_ppRenderTargets[MAX_RENDERTARGETS];
        FrameBufferDesc m_Desc;
    };
}
