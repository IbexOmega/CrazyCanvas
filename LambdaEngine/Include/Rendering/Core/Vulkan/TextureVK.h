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

		bool Init(const TextureDesc& desc);
		void InitWithImage(VkImage image, const TextureDesc& desc);

        FORCEINLINE VkImage GetImage() const
        {
            return m_Image;
        }

        // IDeviceChild interface
        virtual void SetName(const char* pName) override final;

        //´ITexture interface
        FORCEINLINE virtual TextureDesc GetDesc() const override final
        {
            return m_Desc;
        }

        FORCEINLINE virtual uint64 GetHandle() const override final
        {
            return (uint64)m_Image;
        }

	private:
		VkImage			m_Image		= VK_NULL_HANDLE;
		VkDeviceMemory	m_Memory	= VK_NULL_HANDLE;
        
		TextureDesc m_Desc;
	};
}
