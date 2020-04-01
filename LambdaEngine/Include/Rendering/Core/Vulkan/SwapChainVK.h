#pragma once
#include <vector>

#include "Rendering/Core/API/ISwapChain.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
    class TextureVK;
    class GraphicsDeviceVK;

    class SwapChainVK : public DeviceChildBase<GraphicsDeviceVK, ISwapChain>
    {
        using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ISwapChain>;
        
    public:
        SwapChainVK(const GraphicsDeviceVK* pDevice);
        ~SwapChainVK();
        
        bool Init(const Window* pWindow, const SwapChainDesc& desc);
        
        virtual bool ResizeBuffers(uint32 width, uint32 height) override final;
        virtual void Present()                                  override final;
        
        virtual void SetName(const char* pName) override final;
    
        virtual ITexture* GetBuffer(uint32 bufferIndex) override final;
        
        FORCEINLINE virtual const Window* GetWindow() const override final
        {
            return m_pWindow;
        }
        
        FORCEINLINE virtual SwapChainDesc GetDesc() const override final
        {
            return m_Desc;
        }
        
    private:
        bool InitSwapChain(uint32 width, uint32 height);
        void ReleaseResources();
        
    private:
        const Window*  m_pWindow   = nullptr;
        
        VkSurfaceKHR    m_Surface           = VK_NULL_HANDLE;
        VkSwapchainKHR  m_SwapChain         = VK_NULL_HANDLE;
        uint32          m_SemaphoreIndex    = 0;

        VkPresentModeKHR    m_PresentationMode;
        VkSurfaceFormatKHR  m_VkFormat;
        
        std::vector<TextureVK*>     m_Buffers;
        std::vector<VkSemaphore>    m_ImageSemaphores;
        std::vector<VkSemaphore>    m_RenderSemaphores;
        
        SwapChainDesc m_Desc;
    };
}
