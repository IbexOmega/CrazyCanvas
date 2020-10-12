#pragma once
#include "Core/TSharedRef.h"

#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class DeviceAllocatorVK;

	class TextureVK : public TDeviceChildBase<GraphicsDeviceVK, Texture>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, Texture>;

	public:
		TextureVK(const GraphicsDeviceVK* pDevice);
		~TextureVK();

		bool Init(const TextureDesc* pDesc);
		void InitWithImage(VkImage image, const TextureDesc* pDesc);

		FORCEINLINE VkImage GetImage() const
		{
			return m_Image;
		}

		FORCEINLINE VkImageAspectFlags GetAspectFlags() const
		{
			return m_AspectFlags;
		}

	public:
		void InternalRelease();

		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// Texture interface
		inline virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Image);
		}

	private:
		VkImage				m_Image = VK_NULL_HANDLE;
		VkImageAspectFlags	m_AspectFlags;
		AllocationVK		m_Allocation;
	};
}
