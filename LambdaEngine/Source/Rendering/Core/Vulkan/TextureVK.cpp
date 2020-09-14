#include "Log/Log.h"

#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"

namespace LambdaEngine
{
	TextureVK::TextureVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
		, m_Allocation()
	{
	}

	TextureVK::~TextureVK()
	{
		InternalRelease();
	}

	bool TextureVK::Init(const TextureDesc* pDesc)
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
		info.samples				= ConvertSampleCount(pDesc->SampleCount);
		info.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
		info.tiling					= VK_IMAGE_TILING_OPTIMAL;
		
		if (pDesc->Flags & FTextureFlag::TEXTURE_FLAG_RENDER_TARGET)
		{
			info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (pDesc->Flags & FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE)
		{
			info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (pDesc->Flags & FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL)
		{
			info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		if (pDesc->Flags & FTextureFlag::TEXTURE_FLAG_UNORDERED_ACCESS)
		{
			info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (pDesc->Flags & FTextureFlag::TEXTURE_FLAG_COPY_DST)
		{
			info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		if (pDesc->Flags & FTextureFlag::TEXTURE_FLAG_COPY_SRC)
		{
			info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		if (pDesc->Flags & FTextureFlag::TEXTURE_FLAG_CUBE_COMPATIBLE)
		{
			info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

		if (pDesc->Type == ETextureType::TEXTURE_TYPE_1D)
		{
			info.imageType = VK_IMAGE_TYPE_1D;
		}
		else if (pDesc->Type == ETextureType::TEXTURE_TYPE_2D)
		{
			info.imageType = VK_IMAGE_TYPE_2D;
		}
		else if (pDesc->Type == ETextureType::TEXTURE_TYPE_3D)
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

			m_Desc = *pDesc;
			SetName(m_Desc.DebugName);
		}

		VkMemoryRequirements memoryRequirements = { };
		vkGetImageMemoryRequirements(m_pDevice->Device, m_Image, &memoryRequirements);

		VkMemoryPropertyFlags memoryProperties = 0;
		if (m_Desc.MemoryType == EMemoryType::MEMORY_TYPE_CPU_VISIBLE)
		{
			memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else if (m_Desc.MemoryType == EMemoryType::MEMORY_TYPE_GPU)
		{
			memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}

		int32 memoryTypeIndex = FindMemoryType(m_pDevice->PhysicalDevice, memoryRequirements.memoryTypeBits, memoryProperties);
		if (!m_pDevice->AllocateTextureMemory(&m_Allocation, memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex))
		{
			LOG_ERROR("[TextureVK]: Failed to allocate memory");
			return false;
		}

		result = vkBindImageMemory(m_pDevice->Device, m_Image, m_Allocation.Memory, m_Allocation.Offset);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[TextureVK]: Failed to bind memory");
			return false;
		}

		return true;
	}

	void TextureVK::InitWithImage(VkImage image, const TextureDesc* pDesc)
	{
		VALIDATE(image != VK_NULL_HANDLE);
		VALIDATE(pDesc != nullptr);
		
		InternalRelease();

		m_Image = image;
		m_Desc = *pDesc;
		
		SetName(m_Desc.DebugName);
		
		D_LOG_MESSAGE("[TextureVK]: Created texture w=%d, h=%d, d=%d", pDesc->Width, pDesc->Height, pDesc->Depth);
	}
	
	void TextureVK::InternalRelease()
	{
		if (m_Allocation.Memory != VK_NULL_HANDLE)
		{
			if (m_Image != VK_NULL_HANDLE)
			{
				vkDestroyImage(m_pDevice->Device, m_Image, nullptr);
				m_Image = VK_NULL_HANDLE;
			}

			m_pDevice->FreeMemory(&m_Allocation);
			ZERO_MEMORY(&m_Allocation, sizeof(m_Allocation));
		}
	}

	void TextureVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName,reinterpret_cast<uint64>(m_Image), VK_OBJECT_TYPE_IMAGE);
		m_Desc.DebugName = debugName;
	}
}
