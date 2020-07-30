#include "Log/Log.h"

#include <algorithm>

#include "Containers/String.h"

#include "Application/API/Window.h"
#include "Application/API/PlatformApplication.h"

#include "Rendering/Core/Vulkan/SwapChainVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/CommandQueueVK.h"

namespace LambdaEngine
{
    SwapChainVK::SwapChainVK(const GraphicsDeviceVK* pDevice)
        : TDeviceChild(pDevice),
        m_Buffers(),
        m_Desc()
    {
    }

    SwapChainVK::~SwapChainVK()
    {
        ReleaseResources();

		if (m_pCommandQueue)
		{
			m_pCommandQueue->FlushBarriers();
			RELEASE(m_pCommandQueue);
		}
    	
        // Destroy semaphores
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
        
        // Destroy surface
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

        const uint32 bufferCount = uint32(m_Buffers.size());
        for (uint32 i = 0; i < bufferCount; i++)
        {
#ifdef LAMBDA_DEVELOPMENT
            uint64 refCount = m_Buffers[i]->Release();
            if (refCount > 0)
            {
                LOG_ERROR("[SwapChainVK]: All external references to all buffers must be released before calling Release or ResizeBuffers");
                DEBUGBREAK();
            }
#else
            m_Buffers[i]->Release();
#endif
            m_Buffers[i] = nullptr;
        }
        m_Buffers.clear();
    }

    bool SwapChainVK::Init(const Window* pWindow, ICommandQueue* pCommandQueue, const SwapChainDesc* pDesc)
    {
        VALIDATE(pWindow        != nullptr);
        VALIDATE(pCommandQueue  != nullptr);
        VALIDATE(pDesc          != nullptr);

        // Create platform specific surface
#if defined(LAMBDA_PLATFORM_MACOS)
        // Create surface for macOS
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
        // Create a surface for windows
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

        if (m_Surface == VK_NULL_HANDLE)
        {
            LOG_ERROR("[SwapChainVK]: Failed to create surface for SwapChain");
            return false;
        }
        else
        {
            D_LOG_MESSAGE("[SwapChainVK]: Created Surface");
        }
        
        // Check for presentationsupport
        VkBool32    presentSupport      = false;
        uint32      queueFamilyIndex    = m_pDevice->GetQueueFamilyIndexFromQueueType(pCommandQueue->GetType());
        vkGetPhysicalDeviceSurfaceSupportKHR(m_pDevice->PhysicalDevice, queueFamilyIndex, m_Surface, &presentSupport);
        if (!presentSupport)
        {
            LOG_ERROR("[SwapChainVK]: Queue does not support presentation");
            return false;
        }

        // Setup semaphore structure
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.pNext = nullptr;
        semaphoreInfo.flags = 0;

        // Create semaphores
        m_ImageSemaphores.resize(pDesc->BufferCount);
        m_RenderSemaphores.resize(pDesc->BufferCount);
        for (uint32 i = 0; i < pDesc->BufferCount; i++)
        {
            VkResult result = vkCreateSemaphore(m_pDevice->Device, &semaphoreInfo, nullptr, &m_ImageSemaphores[i]);
            if (result != VK_SUCCESS)
            {
                LOG_VULKAN_ERROR(result, "[SwapChainVK]: Failed to create Semaphore");
                return false;
            }

            result = vkCreateSemaphore(m_pDevice->Device, &semaphoreInfo, nullptr, &m_RenderSemaphores[i]);
            if (result != VK_SUCCESS)
            {
                LOG_VULKAN_ERROR(result, "[SwapChainVK]: Failed to create Semaphore");
                return false;
            }

            std::string name = std::string(pDesc->pName) + " ImageSemaphore[" + std::to_string(i) + "]";
            m_pDevice->SetVulkanObjectName(name.c_str(), (uint64)m_ImageSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE);
            D_LOG_MESSAGE("[SwapChainVK]: Created Semaphore %p", m_ImageSemaphores[i]);

            name = std::string(pDesc->pName) + " RenderSemaphore[" + std::to_string(i) + "]";
            m_pDevice->SetVulkanObjectName(name.c_str(), (uint64)m_RenderSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE);
            D_LOG_MESSAGE("[SwapChainVK]: Created Semaphore %p", m_RenderSemaphores[i]);
        }

        // Get supported surface formats
        uint32 formatCount = 0;
        VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_pDevice->PhysicalDevice, m_Surface, &formatCount, nullptr);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: Failed to get surface capabilities");
            return false;
        }

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_pDevice->PhysicalDevice, m_Surface, &formatCount, formats.data());

        // Find the swapchain format we want
        VkFormat lookingFor = ConvertFormat(pDesc->Format);
        m_VkFormat = { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        for (const VkSurfaceFormatKHR& availableFormat : formats)
        {
            if (availableFormat.format == lookingFor && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_VkFormat = availableFormat;
                break;
            }
        }

        if (m_VkFormat.format != VK_FORMAT_UNDEFINED)
        {
            D_LOG_MESSAGE("[SwapChainVK]: Chosen SwapChain format '%s'", VkFormatToString(m_VkFormat.format));
        }
        else
        {
            LOG_ERROR("Vulkan: Format %s is not supported on. Following formats is supported for Creating a SwapChain", VkFormatToString(lookingFor));
            for (const VkSurfaceFormatKHR& availableFormat : formats)
            {
                LOG_ERROR("    %s", VkFormatToString(availableFormat.format));
            }

            return false;
        }

        // Get presentation modes
        uint32 presentModeCount = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->PhysicalDevice, m_Surface, &presentModeCount, nullptr);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: Failed to get surface presentation modes");
            return false;
        }

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->PhysicalDevice, m_Surface, &presentModeCount, presentModes.data());

        m_PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
        if (!pDesc->VerticalSync)
        {
            // Search for the mailbox mode
            for (const VkPresentModeKHR& availablePresentMode : presentModes)
            {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    m_PresentationMode = availablePresentMode;
                    break;
                }
            }

            // If mailbox is not available we choose immediete
            if (m_PresentationMode == VK_PRESENT_MODE_FIFO_KHR)
            {
                for (const VkPresentModeKHR& availablePresentMode : presentModes)
                {
                    if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                    {
                        m_PresentationMode = availablePresentMode;
                        break;
                    }
                }
            }
        }

        D_LOG_MESSAGE("[SwapChainVK]: Chosen SwapChain PresentationMode '%s'", VkPresentatModeToString(m_PresentationMode));

        VkSurfaceCapabilitiesKHR capabilities = { };
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pDevice->PhysicalDevice, m_Surface, &capabilities);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: Failed to get surface capabilities");
            return false;
        }

        if (pDesc->BufferCount < capabilities.minImageCount || pDesc->BufferCount > capabilities.maxImageCount)
        {
            LOG_ERROR("[SwapChainVK]: Number of buffers(=%u) is not supported. MinBuffers=%u MaxBuffers=%u", pDesc->BufferCount, capabilities.minImageCount, capabilities.maxImageCount);
            return false;
        }

        D_LOG_MESSAGE("[SwapChainVK]: Number of buffers in SwapChain '%u'", pDesc->BufferCount);
        memcpy(&m_Desc, pDesc, sizeof(m_Desc));
        m_pWindow   = pWindow;

        m_pCommandQueue = reinterpret_cast<CommandQueueVK*>(pCommandQueue);
        m_pCommandQueue->AddRef();

        return InitSwapChain(pDesc->Width, pDesc->Height);
    }

    bool SwapChainVK::InitSwapChain(uint32 width, uint32 height)
    {
        VkSurfaceCapabilitiesKHR capabilities = { };
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pDevice->PhysicalDevice, m_Surface, &capabilities);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: Failed to get surface capabilities");
            return false;
        }

        // Choose swapchain extent (Size)
        VkExtent2D newExtent = {};
        if (capabilities.currentExtent.width    != UINT32_MAX ||
            capabilities.currentExtent.height   != UINT32_MAX ||
            width == 0 || height == 0)
        {
            newExtent = capabilities.currentExtent;
        }
        else
        {
            newExtent.width     = std::max(capabilities.minImageExtent.width,   std::min(capabilities.maxImageExtent.width,     width));
            newExtent.height    = std::max(capabilities.minImageExtent.height,  std::min(capabilities.maxImageExtent.height,    height));
        }
        newExtent.width     = std::max(newExtent.width, 1u);
        newExtent.height    = std::max(newExtent.height, 1u);
        m_Desc.Width    = newExtent.width;
        m_Desc.Height   = newExtent.height;

        D_LOG_MESSAGE("[SwapChainVK]: Chosen SwapChain size w: %u h: %u", newExtent.width, newExtent.height);

        // Create swapchain
        VkSwapchainCreateInfoKHR info = {};
        info.sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.pNext                  = nullptr;
        info.surface                = m_Surface;
        info.minImageCount          = m_Desc.BufferCount;
        info.imageFormat            = m_VkFormat.format;
        info.imageColorSpace        = m_VkFormat.colorSpace;
        info.imageExtent            = newExtent;
        info.imageArrayLayers       = 1;
        info.imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount  = 0;
        info.pQueueFamilyIndices    = nullptr;
        info.imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        info.preTransform           = capabilities.currentTransform;
        info.compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode            = m_PresentationMode;
        info.clipped                = VK_TRUE;
        info.oldSwapchain           = VK_NULL_HANDLE;

        result = vkCreateSwapchainKHR(m_pDevice->Device, &info, nullptr, &m_SwapChain);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: Failed to create SwapChain");
            
            m_SwapChain = VK_NULL_HANDLE;
            return false;
        }
        else
        {
            D_LOG_MESSAGE("[SwapChainVK]: Created SwapChain");
            m_SemaphoreIndex = 0;
        }

        // Get SwapChain images
        uint32 imageCount = 0;
        vkGetSwapchainImagesKHR(m_pDevice->Device, m_SwapChain, &imageCount, nullptr);
        m_Desc.BufferCount = imageCount;

        std::vector<VkImage> textures(imageCount);
        result = vkGetSwapchainImagesKHR(m_pDevice->Device, m_SwapChain, &imageCount, textures.data());
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: Failed to retrive SwapChain-Images");
            return false;
        }

        const char* names[] =
        {
            "BackBuffer[0]",
            "BackBuffer[1]",
            "BackBuffer[2]",
            "BackBuffer[3]",
            "BackBuffer[4]",
        };
        
        for (uint32 i = 0; i < imageCount; i++)
        {
            TextureDesc desc = {};
            desc.Name			= names[i];
            desc.Type           = ETextureType::TEXTURE_2D;
            desc.Flags          = TEXTURE_FLAG_RENDER_TARGET;
            desc.MemoryType     = EMemoryType::MEMORY_GPU;
            desc.Format         = m_Desc.Format;
            desc.Width          = m_Desc.Width;
            desc.Height         = m_Desc.Height;
            desc.Depth          = 1;
            desc.ArrayCount     = 1;
            desc.Miplevels      = 1;
            desc.SampleCount    = 1;

            TextureVK* pTexture = DBG_NEW TextureVK(m_pDevice);
            pTexture->InitWithImage(textures[i], &desc);
            m_Buffers.emplace_back(pTexture);
        }

        result = AquireNextImage();
        if (result != VK_SUCCESS)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    VkResult SwapChainVK::AquireNextImage()
    {
        VkSemaphore semaphore = m_ImageSemaphores[m_SemaphoreIndex];
        VkResult result = vkAcquireNextImageKHR(m_pDevice->Device, m_SwapChain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &m_BackBufferIndex);
        if (result == VK_SUCCESS)
        {
            m_pCommandQueue->AddWaitSemaphore(semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        }
        else
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: vkAcquireNextImageKHR failed");
        }

        return result;
    }

    bool SwapChainVK::Present()
    {
        VkSemaphore waitSemaphores[] = { m_RenderSemaphores[m_SemaphoreIndex] };
        
        // Perform empty submit on queue for signaling the semaphore
        VkSubmitInfo submitInfo = { };
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext                = nullptr;
        submitInfo.commandBufferCount   = 0;
        submitInfo.pCommandBuffers      = nullptr;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = waitSemaphores;
        submitInfo.waitSemaphoreCount   = 0;
        submitInfo.pWaitSemaphores      = nullptr;
        submitInfo.pWaitDstStageMask    = nullptr;

        VkResult result = vkQueueSubmit(m_pCommandQueue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: Submit failed");
            return false;
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext               = nullptr;
        presentInfo.swapchainCount      = 1;
        presentInfo.pSwapchains         = &m_SwapChain;
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = waitSemaphores;
        presentInfo.pResults            = nullptr;
        presentInfo.pImageIndices       = &m_BackBufferIndex;

        result = vkQueuePresentKHR(m_pCommandQueue->GetQueue(), &presentInfo);
        if (result == VK_SUCCESS)
        {
            m_SemaphoreIndex = (m_SemaphoreIndex + 1) % m_Desc.BufferCount;
            result = AquireNextImage();
        }

        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[SwapChainVK]: Present failed");
            return false;
        }

        return true;
    }

    bool SwapChainVK::ResizeBuffers(uint32 width, uint32 height)
    {
        if (width == m_Desc.Width && height == m_Desc.Height)
        {
            return false;
        }

        ReleaseResources();
        return InitSwapChain(width, height);
    }

    ITexture* SwapChainVK::GetBuffer(uint32 bufferIndex)
    {
        ASSERT(bufferIndex < uint32(m_Buffers.size()));

        TextureVK* pBuffer = m_Buffers[bufferIndex];
        pBuffer->AddRef();
        return pBuffer;
    }

    const ITexture* SwapChainVK::GetBuffer(uint32 bufferIndex) const
    {
        ASSERT(bufferIndex < uint32(m_Buffers.size()));

        TextureVK* pBuffer = m_Buffers[bufferIndex];
        pBuffer->AddRef();
        return pBuffer;
    }

    ICommandQueue* SwapChainVK::GetCommandQueue()
    {
        m_pCommandQueue->AddRef();
        return m_pCommandQueue;
    }

    void SwapChainVK::SetName(const char* pName)
    {
        if (pName)
        {
            TDeviceChild::SetName(pName);
            m_pDevice->SetVulkanObjectName(pName, (uint64)m_SwapChain, VK_OBJECT_TYPE_SWAPCHAIN_KHR);

            m_Desc.pName = m_pDebugName;
        }
    }
}
