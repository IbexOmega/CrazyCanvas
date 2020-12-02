#include "Log/Log.h"

#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/TextureVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	TextureViewVK::TextureViewVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	TextureViewVK::~TextureViewVK()
	{
		InternalRelease();
	}

	bool TextureViewVK::Init(const TextureViewDesc* pDesc)
	{
		InternalRelease();

		const TextureVK* pTextureVk = reinterpret_cast<const TextureVK*>(pDesc->pTexture);
		VALIDATE(pDesc->Format == pTextureVk->GetDesc().Format);

		VkImageViewCreateInfo createInfo = { };
		createInfo.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.pNext							= nullptr;
		createInfo.flags							= 0;
		createInfo.format							= ConvertFormat(pDesc->Format);
		createInfo.image							= pTextureVk->GetImage();
		createInfo.components.r						= VK_COMPONENT_SWIZZLE_R;
		createInfo.components.g						= VK_COMPONENT_SWIZZLE_G;
		createInfo.components.b						= VK_COMPONENT_SWIZZLE_B;
		createInfo.components.a						= VK_COMPONENT_SWIZZLE_A;
		createInfo.subresourceRange.baseArrayLayer	= pDesc->ArrayIndex;
		createInfo.subresourceRange.layerCount		= pDesc->ArrayCount;
		createInfo.subresourceRange.baseMipLevel	= pDesc->Miplevel;
		createInfo.subresourceRange.levelCount		= pDesc->MiplevelCount;

		if (pDesc->Flags & FTextureViewFlag::TEXTURE_VIEW_FLAG_DEPTH_STENCIL)
		{
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (
			(pDesc->Flags & FTextureViewFlag::TEXTURE_VIEW_FLAG_RENDER_TARGET)   ||
			(pDesc->Flags & FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE) ||
			(pDesc->Flags & FTextureViewFlag::TEXTURE_VIEW_FLAG_UNORDERED_ACCESS))
		{
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (pDesc->Type == ETextureViewType::TEXTURE_VIEW_TYPE_1D)
		{
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
		}
		else if (pDesc->Type == ETextureViewType::TEXTURE_VIEW_TYPE_2D)
		{
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		}
		else if (pDesc->Type == ETextureViewType::TEXTURE_VIEW_TYPE_2D_ARRAY)
		{
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		}
		else if (pDesc->Type == ETextureViewType::TEXTURE_VIEW_TYPE_3D)
		{
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		}
		else if (pDesc->Type == ETextureViewType::TEXTURE_VIEW_TYPE_CUBE)
		{
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		}
		else if (pDesc->Type == ETextureViewType::TEXTURE_VIEW_TYPE_CUBE_ARRAY)
		{
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		}

		VkResult result = vkCreateImageView(m_pDevice->Device, &createInfo, nullptr, &m_ImageView);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to create view");
			return false;
		}
		else
		{
			m_Desc = *pDesc;
			m_Desc.pTexture->AddRef();
			SetName(pDesc->DebugName);

			LOG_DEBUG("Created ImageView");

			return true;
		}
	}

	void TextureViewVK::InternalRelease()
	{
		if (m_ImageView != VK_NULL_HANDLE)
		{
			SAFERELEASE(m_Desc.pTexture);
			m_pDevice->DestroyImageView(&m_ImageView);
		}
	}

	void TextureViewVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_ImageView), VK_OBJECT_TYPE_IMAGE_VIEW);
		m_Desc.DebugName = debugName;
	}

	Texture* TextureViewVK::GetTexture()
	{
		return m_Desc.pTexture;
	}
}
