#include "Log/Log.h"

#include "Rendering/Core/Vulkan/FrameBufferCacheVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#include <mutex>

namespace LambdaEngine
{
	FrameBufferCacheVK::FrameBufferCacheVK(const GraphicsDeviceVK* pDevice)
		: m_pDevice(pDevice),
		m_FrameBufferMap()
	{
	}

	FrameBufferCacheVK::~FrameBufferCacheVK()
	{
		for (FrameBufferMapEntry& entry : m_FrameBufferMap)
		{
			vkDestroyFramebuffer(m_pDevice->Device, entry.second, nullptr);
			entry.second = VK_NULL_HANDLE;
		}

		m_FrameBufferMap.clear();
	}

	void FrameBufferCacheVK::DestroyRenderPass(VkRenderPass renderPass) const
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		
		for (FrameBufferMap::iterator it = m_FrameBufferMap.begin(); it != m_FrameBufferMap.end();)
		{
			if (it->first.Contains(renderPass))
			{
				vkDestroyFramebuffer(m_pDevice->Device, it->second, nullptr);
				it->second = VK_NULL_HANDLE;

				it = m_FrameBufferMap.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	void FrameBufferCacheVK::DestroyImageView(VkImageView imageView) const
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		
		for (FrameBufferMap::iterator it = m_FrameBufferMap.begin(); it != m_FrameBufferMap.end();)
		{
			if (it->first.Contains(imageView))
			{
				vkDestroyFramebuffer(m_pDevice->Device, it->second, nullptr);
				it->second = VK_NULL_HANDLE;

				it = m_FrameBufferMap.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	VkFramebuffer FrameBufferCacheVK::GetFrameBuffer(const FrameBufferCacheKey& key, uint32 width, uint32 height)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);
		
		// Check if this framebuffer extists and return it
		FrameBufferMap::iterator entry = m_FrameBufferMap.find(key);
		if (entry != m_FrameBufferMap.end())
		{
			return entry->second;
		}

		VkImageView attachments[MAX_COLOR_ATTACHMENTS + 1];
		memcpy(attachments, key.ColorAttachmentsViews, key.ColorAttachmentViewCount * sizeof(VkImageView));
		attachments[key.ColorAttachmentViewCount] = key.DepthStencilView;

		uint32 attachmentCount = key.DepthStencilView == VK_NULL_HANDLE ? 0 : 1;
		attachmentCount += key.ColorAttachmentViewCount;

		//Create a new framebuffer
		VkFramebufferCreateInfo createInfo = { };
		createInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.pNext			= nullptr;
		createInfo.flags			= 0;
		createInfo.width			= width;
		createInfo.height			= height;
		createInfo.layers			= 1;
		createInfo.attachmentCount	= attachmentCount;
		createInfo.pAttachments		= attachments;
		createInfo.renderPass		= key.RenderPass;

		VkFramebuffer frameBuffer = VK_NULL_HANDLE;

		VkResult result = vkCreateFramebuffer(m_pDevice->Device, &createInfo, nullptr, &frameBuffer);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[FrameBufferCacheVK]: Failed to create framebuffer");
			return VK_NULL_HANDLE;
		}
		else
		{
			D_LOG_MESSAGE("[FrameBufferCacheVK]: Created framebuffer [0]:%p [1]:%p [2]:%p [3]:%p [4]:%p [5]:%p [6]:%p [7]:%p [Depth]:%p [Pass]:%p", key.ColorAttachmentsViews[0], key.ColorAttachmentsViews[1], key.ColorAttachmentsViews[2], key.ColorAttachmentsViews[3], key.ColorAttachmentsViews[4], key.ColorAttachmentsViews[5], key.ColorAttachmentsViews[6], key.ColorAttachmentsViews[7], key.DepthStencilView, key.RenderPass);

			m_FrameBufferMap.insert(FrameBufferMapEntry(key, frameBuffer));
			return frameBuffer;
		}
	}
}
