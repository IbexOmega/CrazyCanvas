#include "Application/API/Window.h"

#include "Application/PlatformApplication.h"

#include "Rendering/Core/Vulkan/SwapChainVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

namespace LambdaEngine
{
    SwapChainVK::SwapChainVK(const GraphicsDeviceVK* pDevice)
        :   TDeviceChild(pDevice),
        m_Buffers(),
        m_Desc()
    {
    }

    SwapChainVK::~SwapChainVK()
    {
        ReleaseResources();
        
        //Destroy semaphores
        for (uint32 i = 0; i < m_Desc.BufferCount; i++)
        {
            if (m_ImageSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_pDevice->Device, m_ImageSemaphores[i], nullptr);
                m_ImageSemaphores[i] = VK_NULL_HANDLE;
            }
            
            if (m_RenderSemaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_pDevice->Device, m_RenderSemaphores[i], nullptr);
                m_RenderSemaphores[i] = VK_NULL_HANDLE;
            }
        }
        
        //Destroy surface
        if (m_Surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_pDevice->Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }
    }

    void SwapChainVK::ReleaseResources()
    {
        if (m_SwapChain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_pDevice->Device, m_SwapChain, nullptr);
            m_SwapChain = VK_NULL_HANDLE;
        }
    }

    bool SwapChainVK::Init(const Window* pWindow, const SwapChainDesc& desc)
    {
        //Create window surface
#if defined(LAMBDA_PLATFORM_MACOS)
        //Create surface for macOS
        {
            VkMacOSSurfaceCreateInfoMVK info = {};
            info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
            info.pNext = nullptr;
            info.flags = 0;
            //info.pView = Window.pNSView;
            if (vkCreateMacOSSurfaceMVK(m_pDevice->Instance, &info, nullptr, &m_Surface))
            {
                
            }
        }
#elif defined(LAMBDA_PLATFORM_WINDOWS)
        //Create a surface for windows
        {
            VkWin32SurfaceCreateInfoKHR info = {};
            info.sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            info.pNext      = nullptr;
            info.flags      = 0;
            info.hwnd       = (HWND)pWindow->GetHandle();
            info.hinstance  = PlatformApplication::Get()->GetInstanceHandle();
            if (vkCreateWin32SurfaceKHR(m_pDevice->Instance, &info, nullptr, &m_Surface) != VK_SUCCESS)
            {
                m_Surface = VK_NULL_HANDLE;
            }
        }
#endif
        
        
        
        return true;
    }

    ITexture* SwapChainVK::GetBuffer(uint32 bufferIndex)
    {
        return nullptr;
    }


    void SwapChainVK::SetName(const char* pName)
    {
        m_pDevice->SetVulkanObjectName(pName, (uint64)m_SwapChain, VK_OBJECT_TYPE_SWAPCHAIN_KHR);
    }


    void SwapChainVK::Present()
    {
    }


    bool SwapChainVK::ResizeBuffers(uint32 width, uint32 height)
    {
        return false;
    }
}
