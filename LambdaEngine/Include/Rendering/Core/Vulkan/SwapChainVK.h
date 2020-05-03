#pragma once
#include "Containers/TArray.h"

#include "Rendering/Core/API/ISwapChain.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
    class TextureVK;
    class CommandQueueVK;
    class GraphicsDeviceVK;

    class SwapChainVK : public TDeviceChildBase<GraphicsDeviceVK, ISwapChain>
    {
        using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, ISwapChain>;
        
    public:
        SwapChainVK(const GraphicsDeviceVK* pDevice);
        ~SwapChainVK();
        
        bool Init(const IWindow* pWindow, ICommandQueue* pCommandQueue, const SwapChainDesc* pDesc);
        
        // IDeviceChild interface
        virtual void SetName(const char* pName) override final;
        
        // ISwapChain interface
        virtual bool ResizeBuffers(uint32 width, uint32 height) override final;
        
        virtual bool Present() override final;
        
        virtual ITexture*       GetBuffer(uint32 bufferIndex)       override final;
        virtual const ITexture* GetBuffer(uint32 bufferIndex) const override final;
        
        virtual ICommandQueue* GetCommandQueue() override final;

        FORCEINLINE virtual const IWindow* GetWindow() const override final
        {
            return m_pWindow;
        }
        
        FORCEINLINE virtual uint64 GetCurrentBackBufferIndex() const override final
        {
            return uint64(m_BackBufferIndex);
        }

        FORCEINLINE virtual SwapChainDesc GetDesc() const override final
        {
            return m_Desc;
        }
        
    private:
        bool InitSwapChain(uint32 width, uint32 height);
        
        VkResult    AquireNextImage();
        void        ReleaseResources();
        
    private:
        const IWindow*   m_pWindow       = nullptr;
        CommandQueueVK* m_pCommandQueue = nullptr;

        VkSurfaceKHR    m_Surface           = VK_NULL_HANDLE;
        VkSwapchainKHR  m_SwapChain         = VK_NULL_HANDLE;
        
        uint32  m_BackBufferIndex = 0;
        uint32  m_SemaphoreIndex  = 0;

        VkPresentModeKHR    m_PresentationMode;
        VkSurfaceFormatKHR  m_VkFormat;
        
        std::vector<TextureVK*>     m_Buffers;
        std::vector<VkSemaphore>    m_ImageSemaphores;
        std::vector<VkSemaphore>    m_RenderSemaphores;
        
        SwapChainDesc m_Desc;
    };
}
