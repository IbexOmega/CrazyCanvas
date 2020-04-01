#include "Log/Log.h"

#include <string>

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
        //Create platform specific surface
#if defined(LAMBDA_PLATFORM_MACOS)
        //Create surface for macOS
        {
            VkMacOSSurfaceCreateInfoMVK info = {};
            info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
            info.pNext = nullptr;
            info.flags = 0;
            info.pView = pWindow->GetView();
            if (vkCreateMacOSSurfaceMVK(m_pDevice->Instance, &info, nullptr, &m_Surface))
            {
                m_Surface = VK_NULL_HANDLE;
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
            info.hinstance  = PlatformApplication::GetInstanceHandle();
            if (vkCreateWin32SurfaceKHR(m_pDevice->Instance, &info, nullptr, &m_Surface) != VK_SUCCESS)
            {
                m_Surface = VK_NULL_HANDLE;
            }
        }
#endif

        if (m_Surface == VK_NULL_HANDLE)
        {
            LOG_ERROR("[SwapChainVK]: Failed to create surface for SwapChain");
            return false;
        }
        else
        {
            D_LOG_MESSAGE("[SwapChainVK]: Created Surface");
        }
        
        //Check for presentationsupport
        VkBool32            presentSupport  = false;
        QueueFamilyIndices  familyIndices   = m_pDevice->GetQueueFamilyIndices();
        vkGetPhysicalDeviceSurfaceSupportKHR(m_pDevice->PhysicalDevice, familyIndices.PresentFamily, m_Surface, &presentSupport);
        if (!presentSupport)
        {
            LOG_ERROR("[SwapChainVK]: Queuefamily does not support presentation");
            return false;
        }

        //Setup semaphore structure
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.pNext = nullptr;
        semaphoreInfo.flags = 0;

        //Create semaphores
        m_ImageSemaphores.resize(desc.BufferCount);
        m_RenderSemaphores.resize(desc.BufferCount);
        for (uint32 i = 0; i < desc.BufferCount; i++)
        {
            if (vkCreateSemaphore(m_pDevice->Device, &semaphoreInfo, nullptr, &m_ImageSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_pDevice->Device, &semaphoreInfo, nullptr, &m_RenderSemaphores[i]) != VK_SUCCESS)
            {
                LOG_ERROR("[SwapChainVK]: Failed to create Semaphore");
                return false;
            }
            else
            {
                std::string name = std::string(desc.pName) + " ImageSemaphore[" + std::to_string(i) + "]";
                m_pDevice->SetVulkanObjectName(name.c_str(), (uint64)m_ImageSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE);
                D_LOG_MESSAGE("[SwapChainVK]: Created Semaphore %p", m_ImageSemaphores[i]);

                name = std::string(desc.pName) + " RenderSemaphore[" + std::to_string(i) + "]";
                m_pDevice->SetVulkanObjectName(name.c_str(), (uint64)m_RenderSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE);
                D_LOG_MESSAGE("[SwapChainVK]: Created Semaphore %p", m_RenderSemaphores[i]);
            }
        }

        ////Get the swapchain capabilities from the adapter
        //SwapChainCapabilities cap = { };
        //VkResult result = QuerySwapChainSupport(cap, m_pDevice->GetVkPhysicalDevice(), m_Surface);
        //if (result != VK_SUCCESS)
        //{
        //    LOG_DEBUG_ERROR("Vulkan: QuerySwapChainSupport failed. Error: %s\n", VkResultToString(result));
        //    return;
        //}


        ////Find the swapchain format we want
        //VkFormat lookingFor = ConvertFormat(desc.BufferFormat);
        //m_VkFormat = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        //for (const auto& availableFormat : cap.Formats)
        //{
        //    if (availableFormat.format == lookingFor && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        //    {
        //        m_VkFormat = availableFormat;
        //        break;
        //    }
        //}

        ////Did we find it?
        //if (m_VkFormat.format != VK_FORMAT_UNDEFINED)
        //{
        //    LOG_DEBUG_INFO("Vulkan: Chosen SwapChain format '%s'\n", VkFormatToString(m_VkFormat.format));
        //}
        //else
        //{
        //    LOG_DEBUG_ERROR("Vulkan: Format %s is not supported on. Following formats is supported for Creating a SwapChain\n", VkFormatToString(lookingFor));
        //    for (const auto& availableFormat : cap.Formats)
        //        LOG_DEBUG_ERROR("    %s\n", VkFormatToString(availableFormat.format));

        //    return;
        //}

        ////Choose a presentationmode
        //m_PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
        //if (!desc.VerticalSync)
        //{
        //    //Search for the mailbox mode
        //    for (const auto& availablePresentMode : cap.PresentModes)
        //    {
        //        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        //        {
        //            m_PresentationMode = availablePresentMode;
        //            break;
        //        }
        //    }

        //    //If mailbox is not available we choose immediete
        //    if (m_PresentationMode == VK_PRESENT_MODE_FIFO_KHR)
        //    {
        //        for (const auto& availablePresentMode : cap.PresentModes)
        //        {
        //            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        //            {
        //                m_PresentationMode = availablePresentMode;
        //                break;
        //            }
        //        }
        //    }
        //}

        //LOG_DEBUG_INFO("Vulkan: Chosen SwapChain PresentationMode '%s'\n", VkPresentatModeToString(m_PresentationMode));

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
