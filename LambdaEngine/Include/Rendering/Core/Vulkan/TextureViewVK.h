#pragma once
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class TextureVK;
	class GraphicsDeviceVK;

	class TextureViewVK : public TDeviceChildBase<GraphicsDeviceVK, TextureView>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, TextureView>;
		
	public:
		TextureViewVK(const GraphicsDeviceVK* pDevice);
		~TextureViewVK();
		
		bool Init(const TextureViewDesc* pDesc);
		
		FORCEINLINE VkImageView GetImageView() const
		{
			return m_ImageView;
		}
		
	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;
		
		// TextureView interface
		virtual Texture* GetTexture() override final;
		
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_ImageView);
		}
		
	private:
		VkImageView	m_ImageView	= VK_NULL_HANDLE;
	};
}
