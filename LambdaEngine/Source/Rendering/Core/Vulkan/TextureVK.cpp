#include "Log/Log.h"

#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	TextureVK::TextureVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_Desc()
	{
	}

	TextureVK::~TextureVK()
	{
		if (m_Memory != VK_NULL_HANDLE || m_pAllocator != nullptr)
		{
			if (m_Image != VK_NULL_HANDLE)
			{
				vkDestroyImage(m_pDevice->Device, m_Image, nullptr);
				m_Image = VK_NULL_HANDLE;
			}
		}

		if (m_pAllocator)
		{
			m_pAllocator->Free(&m_Allocation);
			memset(&m_Allocation, 0, sizeof(m_Allocation));

			RELEASE(m_pAllocator);
		}
		else
		{
			if (m_Memory != VK_NULL_HANDLE)
			{
				vkFreeMemory(m_pDevice->Device, m_Memory, nullptr);
				m_Memory = VK_NULL_HANDLE;
			}
		}
	}

	bool TextureVK::Init(const TextureDesc* pDesc, IDeviceAllocator* pAllocator)
	{
		VkImageCreateInfo info = {};
		info.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext					= nullptr;
		info.flags					= 0;
		info.format					= ConvertFormat(pDesc->Format);
		info.extent.width			= pDesc->Width;
		info.extent.height			= pDesc->Height;
		info.extent.depth			= pDesc->Depth;
		info.arrayLayers			= pDesc->ArrayCount;
		info.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
		info.mipLevels				= pDesc->Miplevels;
		info.pQueueFamilyIndices	= nullptr;
		info.queueFamilyIndexCount	= 0;
		info.samples				= ConvertSamples(pDesc->SampleCount);
		info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
		info.tiling					= VK_IMAGE_TILING_OPTIMAL;
        
		if (pDesc->Flags & FTextureFlags::TEXTURE_FLAG_RENDER_TARGET)
		{
			info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (pDesc->Flags & FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE)
		{
			info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (pDesc->Flags & FTextureFlags::TEXTURE_FLAG_DEPTH_STENCIL)
		{
			info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		if (pDesc->Flags & FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS)
		{
			info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (pDesc->Flags & FTextureFlags::TEXTURE_FLAG_COPY_DST)
		{
			info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		if (pDesc->Flags & FTextureFlags::TEXTURE_FLAG_COPY_SRC)
		{
			info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if (pDesc->Type == ETextureType::TEXTURE_1D)
		{
			info.imageType = VK_IMAGE_TYPE_1D;
		}
		else if (pDesc->Type == ETextureType::TEXTURE_2D)
		{
			info.imageType = VK_IMAGE_TYPE_2D;
		}
		else if (pDesc->Type == ETextureType::TEXTURE_3D)
		{
			info.imageType = VK_IMAGE_TYPE_3D;
		}

		VkResult result = vkCreateImage(m_pDevice->Device, &info, nullptr, &m_Image);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("[TextureVK]: Failed to create texture");
			return false;
		}
		else
		{
			D_LOG_MESSAGE("[TextureVK]: Created texture w=%d, h=%d, d=%d", pDesc->Width, pDesc->Height, pDesc->Depth);

            memcpy(&m_Desc, pDesc, sizeof(m_Desc));
			SetName(m_Desc.pName);
		}

		VkMemoryRequirements memoryRequirements = { };
		vkGetImageMemoryRequirements(m_pDevice->Device, m_Image, &memoryRequirements);

		VkMemoryPropertyFlags memoryProperties = 0;
		if (m_Desc.MemoryType == EMemoryType::MEMORY_CPU_VISIBLE)
		{
			memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else if (m_Desc.MemoryType == EMemoryType::MEMORY_GPU)
		{
			memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}

		int32 memoryTypeIndex = FindMemoryType(m_pDevice->PhysicalDevice, memoryRequirements.memoryTypeBits, memoryProperties);
		if (pAllocator)
		{
			DeviceAllocatorVK* pAllocatorVk = reinterpret_cast<DeviceAllocatorVK*>(pAllocator);
			if (!pAllocatorVk->Allocate(&m_Allocation, memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex))
			{
				LOG_ERROR("[TextureVK]: Failed to allocate memory");
				return false;
			}

			pAllocatorVk->AddRef();
			m_pAllocator = pAllocatorVk;

			result = vkBindImageMemory(m_pDevice->Device, m_Image, m_Allocation.Memory, m_Allocation.Offset);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[TextureVK]: Failed to bind memory");
				return false;
			}
		}
		else
		{
			result = m_pDevice->AllocateMemory(&m_Memory, memoryRequirements.size, memoryTypeIndex);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[TextureVK]: Failed to allocate memory");
				return false;
			}

			result = vkBindImageMemory(m_pDevice->Device, m_Image, m_Memory, 0);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[TextureVK]: Failed to bind memory");
				return false;
			}
		}

		return true;
	}

	void TextureVK::InitWithImage(VkImage image, const TextureDesc* pDesc)
	{
        VALIDATE(image != VK_NULL_HANDLE);
        VALIDATE(pDesc != nullptr);
        
		m_Image = image;
        memcpy(&m_Desc, pDesc, sizeof(m_Desc));
        
		SetName(m_Desc.pName);
        
        D_LOG_MESSAGE("[TextureVK]: Created texture w=%d, h=%d, d=%d", pDesc->Width, pDesc->Height, pDesc->Depth);
	}
	
	void TextureVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Image, VK_OBJECT_TYPE_IMAGE);

            m_Desc.pName = m_pDebugName;
		}
	}
}
