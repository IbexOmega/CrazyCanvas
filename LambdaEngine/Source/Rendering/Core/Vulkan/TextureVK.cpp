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
		if (m_Image != VK_NULL_HANDLE)
		{
			vkDestroyImage(m_pDevice->Device, m_Image, nullptr);
			m_Image = VK_NULL_HANDLE;
		}

		if (m_Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_pDevice->Device, m_Memory, nullptr);
			m_Memory = VK_NULL_HANDLE;
		}
	}

	bool TextureVK::Create(const TextureDesc& desc)
	{
		VkImageCreateInfo info = {};
		info.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext					= nullptr;
		info.flags					= 0;
		info.format					= ConvertFormat(desc.Format);
		info.extent.width			= desc.Width;
		info.extent.height			= desc.Height;
		info.extent.depth			= desc.Depth;
		info.arrayLayers			= desc.ArrayCount;
		//info.initialLayout = ;
		//info.mipLevels = ;
		//info.pQueueFamilyIndices = nullptr;
		//info.queueFamilyIndexCount = 0;
		//info.samples = 0;
		//info.sharingMode = ;
		//info.tiling = ;
		//info.usage = ;

		//if (desc.Type == ETextureType::TEXTURE_1D)
		//info.imageType = ;

		return true;
	}
}