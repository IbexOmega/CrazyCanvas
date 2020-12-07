#include "Log/Log.h"

#include <algorithm>

#include "Containers/String.h"

#include "Application/API/Window.h"
#include "Application/API/PlatformApplication.h"

#include "Rendering/Core/Vulkan/SwapChainVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/CommandQueueVK.h"

#include "Rendering/RenderAPI.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/RenderEvents.h"

namespace LambdaEngine
{
	SwapChainVK::SwapChainVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
		, m_Buffers()
	{
	}

	SwapChainVK::~SwapChainVK()
	{
		ReleaseInternal();
		ReleaseSurface();

		const uint32 bufferCount = uint32(m_Buffers.GetSize());
		for (uint32 i = 0; i < bufferCount; i++)
		{
#ifdef LAMBDA_DEVELOPMENT
			uint64 bufferViewRefCount	= m_BufferViews[i]->Release();
			uint64 bufferRefCount		= m_Buffers[i]->Release();
			if (bufferRefCount > 0 || bufferViewRefCount > 0)
			{
				LOG_ERROR("All external references to all buffers must be released before calling Release or ResizeBuffers");
				DEBUGBREAK();
			}
#else
			m_Buffers[i]->Release();
			m_BufferViews[i]->Release();
#endif
			m_Buffers[i]		= nullptr;
			m_BufferViews[i]	= nullptr;
		}
		m_Buffers.Clear();
		m_BufferViews.Clear();

		VALIDATE(m_Desc.pQueue != nullptr);
		reinterpret_cast<CommandQueueVK*>(m_Desc.pQueue)->FlushBarriers();
		m_Desc.pQueue->Release();
		m_Desc.pWindow->Release();

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
	}

	void SwapChainVK::ReleaseInternal()
	{
		// Destroy SwapChain
		if (m_SwapChain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(m_pDevice->Device, m_SwapChain, nullptr);
			m_SwapChain = VK_NULL_HANDLE;
		}
	}

	void SwapChainVK::ReleaseSurface()
	{
		// Destroy surface
		if (m_Surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_pDevice->Instance, m_Surface, nullptr);
			m_Surface = VK_NULL_HANDLE;
		}
	}

	VkResult SwapChainVK::HandleOutOfDate()
	{
		CommandQueueVK* pVkQueue = reinterpret_cast<CommandQueueVK*>(m_Desc.pQueue);
		pVkQueue->Flush();

		PreSwapChainRecreatedEvent preEvent(m_Desc.Width, m_Desc.Height);
		EventQueue::SendEventImmediate(preEvent);

		ReleaseInternal();
		ReleaseSurface();

		VkResult result = InitSurface();
		if (result != VK_SUCCESS)
		{
			return result;
		}

		result = InitInternal();
		if (result != VK_SUCCESS)
		{
			return result;
		}

		PostSwapChainRecreatedEvent postEvent(m_Desc.Width, m_Desc.Height);
		EventQueue::SendEventImmediate(postEvent);

		AquireNextBufferIndex();
		return AquireNextImage();
	}

	bool SwapChainVK::Init(const SwapChainDesc* pDesc)
	{
		VALIDATE(pDesc			!= nullptr);
		VALIDATE(pDesc->pWindow	!= nullptr);
		VALIDATE(pDesc->pQueue	!= nullptr);

		EventQueue::RegisterEventHandler(this, &SwapChainVK::OnWindowResized);

		// Setup semaphore structure
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = nullptr;
		semaphoreInfo.flags = 0;

		// Create semaphores
		m_ImageSemaphores.Resize(pDesc->BufferCount);
		m_RenderSemaphores.Resize(pDesc->BufferCount);
		for (uint32 i = 0; i < pDesc->BufferCount; i++)
		{
			VkResult result = vkCreateSemaphore(m_pDevice->Device, &semaphoreInfo, nullptr, &m_ImageSemaphores[i]);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "Failed to create Semaphore");
				return false;
			}

			result = vkCreateSemaphore(m_pDevice->Device, &semaphoreInfo, nullptr, &m_RenderSemaphores[i]);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "Failed to create Semaphore");
				return false;
			}

			String name = pDesc->DebugName + " ImageSemaphore[" + std::to_string(i) + "]";
			m_pDevice->SetVulkanObjectName(name, reinterpret_cast<uint64>(m_ImageSemaphores[i]), VK_OBJECT_TYPE_SEMAPHORE);
			LOG_DEBUG("Created Semaphore %p", m_ImageSemaphores[i]);

			name = pDesc->DebugName + " RenderSemaphore[" + std::to_string(i) + "]";
			m_pDevice->SetVulkanObjectName(name, reinterpret_cast<uint64>(m_RenderSemaphores[i]), VK_OBJECT_TYPE_SEMAPHORE);
			LOG_DEBUG("Created Semaphore %p", m_RenderSemaphores[i]);
		}

		m_Desc = *pDesc;
		m_Desc.pWindow->AddRef();
		m_Desc.pQueue->AddRef();

		if (InitSurface() != VK_SUCCESS)
		{
			return false;
		}

		if (InitInternal() != VK_SUCCESS)
		{
			return false;
		}

		return true;
	}

	bool SwapChainVK::OnWindowResized(const WindowResizedEvent& event)
	{
		ResizeBuffers(event.Width, event.Height);
		return true;
	}

	VkResult SwapChainVK::InitInternal()
	{
		VkExtent2D newSize = GetSizeFromSurface(m_Desc.Width, m_Desc.Height);
		m_Desc.Width	= newSize.width;
		m_Desc.Height	= newSize.height;

		LOG_DEBUG("Chosen SwapChain size w: %u h: %u", newSize.width, newSize.height);

		VkSurfaceCapabilitiesKHR capabilities = { };
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pDevice->PhysicalDevice, m_Surface, &capabilities);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to get surface capabilities");
			return result;
		}

		if (m_Desc.BufferCount < capabilities.minImageCount || m_Desc.BufferCount > capabilities.maxImageCount)
		{
			LOG_ERROR("Number of buffers(=%u) is not supported. MinBuffers=%u MaxBuffers=%u", m_Desc.BufferCount, capabilities.minImageCount, capabilities.maxImageCount);
			return VK_ERROR_UNKNOWN;
		}

		LOG_DEBUG("Number of buffers in SwapChain '%u'", m_Desc.BufferCount);

		// Create swapchain
		VkSwapchainCreateInfoKHR info = { };
		info.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.pNext					= nullptr;
		info.surface				= m_Surface;
		info.minImageCount			= m_Desc.BufferCount;
		info.imageFormat			= m_VkFormat.format;
		info.imageColorSpace		= m_VkFormat.colorSpace;
		info.imageExtent			= newSize;
		info.imageArrayLayers		= 1;
		info.imageSharingMode		= VK_SHARING_MODE_EXCLUSIVE;
		info.queueFamilyIndexCount	= 0;
		info.pQueueFamilyIndices	= nullptr;
		info.imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		info.preTransform			= capabilities.currentTransform;
		info.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.presentMode			= m_PresentationMode;
		info.clipped				= VK_TRUE;
		info.oldSwapchain			= VK_NULL_HANDLE;

		result = vkCreateSwapchainKHR(m_pDevice->Device, &info, nullptr, &m_SwapChain);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to create SwapChain");

			m_SwapChain = VK_NULL_HANDLE;
			return result;
		}
		else
		{
			LOG_DEBUG("Created SwapChain");
			m_SemaphoreIndex = 0;
		}

		// Get SwapChain images
		uint32 imageCount = 0;
		vkGetSwapchainImagesKHR(m_pDevice->Device, m_SwapChain, &imageCount, nullptr);
		m_Desc.BufferCount = imageCount;

		TArray<VkImage> textures(imageCount);
		result = vkGetSwapchainImagesKHR(m_pDevice->Device, m_SwapChain, &imageCount, textures.GetData());
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to retrive SwapChain-Images");
			return result;
		}

		m_Buffers.Resize(imageCount);
		m_BufferViews.Resize(imageCount);
		for (uint32 i = 0; i < imageCount; i++)
		{
			TextureDesc textureDesc = { };
			textureDesc.DebugName		= m_Desc.DebugName + " Texture " + std::to_string(i);
			textureDesc.Type			= ETextureType::TEXTURE_TYPE_2D;
			textureDesc.Flags			= TEXTURE_FLAG_RENDER_TARGET;
			textureDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
			textureDesc.Format			= m_Desc.Format;
			textureDesc.Width			= m_Desc.Width;
			textureDesc.Height			= m_Desc.Height;
			textureDesc.Depth			= 1;
			textureDesc.ArrayCount		= 1;
			textureDesc.Miplevels		= 1;
			textureDesc.SampleCount		= 1;

			if (!m_Buffers[i])
			{
				m_Buffers[i] = DBG_NEW TextureVK(m_pDevice);
			}

			m_Buffers[i]->InitWithImage(textures[i], &textureDesc);

			TextureViewDesc textureViewDesc = {};
			textureViewDesc.DebugName		= m_Desc.DebugName + " Texture View " + std::to_string(i);
			textureViewDesc.pTexture		= m_Buffers[i];
			textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_RENDER_TARGET;
			textureViewDesc.Format			= m_Desc.Format;
			textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			textureViewDesc.MiplevelCount	= 1;
			textureViewDesc.ArrayCount		= 1;
			textureViewDesc.Miplevel		= 0;
			textureViewDesc.ArrayIndex		= 0;

			if (!m_BufferViews[i])
			{
				m_BufferViews[i] = DBG_NEW TextureViewVK(m_pDevice);
			}

			m_BufferViews[i]->Init(&textureViewDesc);
		}

		return AquireNextImage();
	}

	VkResult SwapChainVK::InitSurface()
	{
		VkResult result = VK_SUCCESS;
		ReleaseSurface();

		// Create platform specific surface
#if defined(LAMBDA_PLATFORM_MACOS)
		// Create surface for macOS
		{
			VkMacOSSurfaceCreateInfoMVK info = {};
			info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
			info.pNext = nullptr;
			info.flags = 0;
			info.pView = m_Desc.Window->GetView();

			result = vkCreateMacOSSurfaceMVK(m_pDevice->Instance, &info, nullptr, &m_Surface);
			if (result != VK_SUCCESS)
			{
				m_Surface = VK_NULL_HANDLE;
			}
		}
#elif defined(LAMBDA_PLATFORM_WINDOWS)
		// Create a surface for windows
		{
			VkWin32SurfaceCreateInfoKHR info = {};
			info.sType		= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			info.pNext		= nullptr;
			info.flags		= 0;
			info.hwnd		= reinterpret_cast<HWND>(m_Desc.pWindow->GetHandle());
			info.hinstance	= PlatformApplication::Get().GetInstanceHandle();

			result = vkCreateWin32SurfaceKHR(m_pDevice->Instance, &info, nullptr, &m_Surface);
			if (result != VK_SUCCESS)
			{
				m_Surface = VK_NULL_HANDLE;
			}
		}
#endif

		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to create surface for SwapChain");
			return result;
		}
		else
		{
			LOG_DEBUG("Created Surface");
		}

		// Check for presentationsupport
		VkBool32 presentSupport = false;
		uint32 queueFamilyIndex = m_pDevice->GetQueueFamilyIndexFromQueueType(m_Desc.pQueue->GetType());
		vkGetPhysicalDeviceSurfaceSupportKHR(m_pDevice->PhysicalDevice, queueFamilyIndex, m_Surface, &presentSupport);
		if (!presentSupport)
		{
			LOG_ERROR("Queue does not support presentation");
			return VK_ERROR_UNKNOWN;
		}

		// Get supported surface formats
		uint32 formatCount = 0;
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_pDevice->PhysicalDevice, m_Surface, &formatCount, nullptr);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to get surface capabilities");
			return result;
		}

		TArray<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_pDevice->PhysicalDevice, m_Surface, &formatCount, formats.GetData());

		// Find the swapchain format we want
		VkFormat lookingFor = ConvertFormat(m_Desc.Format);
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
			LOG_DEBUG("Chosen SwapChain format '%s'", VkFormatToString(m_VkFormat.format));
		}
		else
		{
			LOG_ERROR("Vulkan: Format %s is not supported on. The following formats are supported for Creating a SwapChain", VkFormatToString(lookingFor));
			for (const VkSurfaceFormatKHR& availableFormat : formats)
			{
				LOG_ERROR("    %s", VkFormatToString(availableFormat.format));
			}

			return result;
		}

		// Get presentation modes
		uint32 presentModeCount = 0;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->PhysicalDevice, m_Surface, &presentModeCount, nullptr);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to get surface presentation modes");
			return result;
		}

		TArray<VkPresentModeKHR> presentModes(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_pDevice->PhysicalDevice, m_Surface, &presentModeCount, presentModes.GetData());

		m_PresentationMode = VK_PRESENT_MODE_FIFO_KHR;
		if (!m_Desc.VerticalSync)
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

		LOG_DEBUG("Chosen SwapChain PresentationMode '%s'", VkPresentatModeToString(m_PresentationMode));

		return result;
	}

	VkExtent2D SwapChainVK::GetSizeFromSurface(uint32 width, uint32 height)
	{
		VkExtent2D extent = { 0, 0 };

		VkSurfaceCapabilitiesKHR capabilities = { };
		VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_pDevice->PhysicalDevice, m_Surface, &capabilities);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to get surface capabilities");
			return extent;
		}

		// Choose swapchain extent (Size)
		VkExtent2D newExtent = {};
		if (capabilities.currentExtent.width != UINT32_MAX ||
			capabilities.currentExtent.height != UINT32_MAX ||
			width == 0 || height == 0)
		{
			newExtent = capabilities.currentExtent;
		}
		else
		{
			newExtent.width		= std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width));
			newExtent.height	= std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, height));
		}
		newExtent.width		= std::max(newExtent.width, 1u);
		newExtent.height	= std::max(newExtent.height, 1u);
		extent.width	= newExtent.width;
		extent.height	= newExtent.height;

		return extent;
	}

	VkResult SwapChainVK::AquireNextImage()
	{
		VkSemaphore semaphore = m_ImageSemaphores[m_SemaphoreIndex];
		VkResult result = vkAcquireNextImageKHR(m_pDevice->Device, m_SwapChain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &m_BackBufferIndex);
		if (result == VK_SUCCESS)
		{
			reinterpret_cast<CommandQueueVK*>(m_Desc.pQueue)->AddWaitSemaphore(semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		}
		else
		{
			LOG_VULKAN_ERROR(result, "vkAcquireNextImageKHR failed");
		}

		return result;
	}

	bool SwapChainVK::Present()
	{
		if (!m_Desc.pWindow->IsValid())
		{
			return false;
		}

		VkSemaphore waitSemaphores[] = { m_RenderSemaphores[m_SemaphoreIndex] };

		// Perform empty submit on queue for signaling the semaphore
		VkSubmitInfo submitInfo = { };
		submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext				= nullptr;
		submitInfo.commandBufferCount	= 0;
		submitInfo.pCommandBuffers		= nullptr;
		submitInfo.signalSemaphoreCount	= 1;
		submitInfo.pSignalSemaphores	= waitSemaphores;
		submitInfo.waitSemaphoreCount	= 0;
		submitInfo.pWaitSemaphores		= nullptr;
		submitInfo.pWaitDstStageMask	= nullptr;

		VkResult result = vkQueueSubmit(reinterpret_cast<CommandQueueVK*>(m_Desc.pQueue)->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Submit failed");
			return false;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext				= nullptr;
		presentInfo.swapchainCount		= 1;
		presentInfo.pSwapchains			= &m_SwapChain;
		presentInfo.waitSemaphoreCount	= 1;
		presentInfo.pWaitSemaphores		= waitSemaphores;
		presentInfo.pResults			= nullptr;
		presentInfo.pImageIndices		= &m_BackBufferIndex;

		result = vkQueuePresentKHR(reinterpret_cast<CommandQueueVK*>(m_Desc.pQueue)->GetQueue(), &presentInfo);
		if (result == VK_SUCCESS)
		{
			AquireNextBufferIndex();
			result = AquireNextImage();
		}
		else if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			result = HandleOutOfDate();
		}

		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Present failed");
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

		VkExtent2D newExtent = GetSizeFromSurface(width, height);
		if (newExtent.width == m_Desc.Width && newExtent.height == m_Desc.Height)
		{
			return false;
		}

		m_Desc.Width	= width;
		m_Desc.Height	= height;
		m_Desc.pQueue->Flush();

		PreSwapChainRecreatedEvent preEvent(m_Desc.Width, m_Desc.Height);
		EventQueue::SendEventImmediate(preEvent);
		ReleaseInternal();

		if (InitInternal() == VK_SUCCESS)
		{
			PostSwapChainRecreatedEvent postEvent(m_Desc.Width, m_Desc.Height);
			EventQueue::SendEventImmediate(postEvent);
			return true;
		}

		return false;
	}

	Texture* SwapChainVK::GetBuffer(uint32 bufferIndex)
	{
		VALIDATE(bufferIndex < uint32(m_Buffers.GetSize()));

		TextureVK* pBuffer = m_Buffers[bufferIndex];
		pBuffer->AddRef();
		return pBuffer;
	}

	const Texture* SwapChainVK::GetBuffer(uint32 bufferIndex) const
	{
		VALIDATE(bufferIndex < uint32(m_Buffers.GetSize()));

		TextureVK* pBuffer = m_Buffers[bufferIndex];
		pBuffer->AddRef();
		return pBuffer;
	}

	TextureView* SwapChainVK::GetBufferView(uint32 bufferIndex)
	{
		VALIDATE(bufferIndex < uint32(m_BufferViews.GetSize()));

		TextureViewVK* pBufferView = m_BufferViews[bufferIndex];
		pBufferView->AddRef();
		return pBufferView;
	}

	const TextureView* SwapChainVK::GetBufferView(uint32 bufferIndex) const
	{
		VALIDATE(bufferIndex < uint32(m_BufferViews.GetSize()));

		TextureViewVK* pBufferView = m_BufferViews[bufferIndex];
		pBufferView->AddRef();
		return pBufferView;
	}

	void SwapChainVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_SwapChain), VK_OBJECT_TYPE_SWAPCHAIN_KHR);
		m_Desc.DebugName = debugName;
	}
}
