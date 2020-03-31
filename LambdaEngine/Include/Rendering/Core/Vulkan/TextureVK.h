#pragma once
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class TextureVK : public DeviceChildBase<GraphicsDeviceVK, ITexture>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ITexture>;

	public:
		TextureVK(const GraphicsDeviceVK* pDevice);
		~TextureVK();

		bool Create(const TextureDesc& desc);

		FORCEINLINE virtual TextureDesc GetDesc() const override
		{
			return m_Desc;
		}

	private:
		VkImage			m_Image		= VK_NULL_HANDLE;
		VkDeviceMemory	m_Memory	= VK_NULL_HANDLE;
		TextureDesc		m_Desc;
	};
}