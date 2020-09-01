#pragma once
#include "Core/Ref.h"

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

		bool Init(const TextureDesc* pDesc, DeviceAllocator* pAllocator);
		void InitWithImage(VkImage image, const TextureDesc* pDesc);

		FORCEINLINE VkImage GetImage() const
		{
			return m_Image;
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// Texture interface
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Image);
		}

	private:
		TSharedRef<DeviceAllocatorVK>	m_Allocator	= nullptr;
		VkImage					m_Image		= VK_NULL_HANDLE;
		VkDeviceMemory			m_Memory	= VK_NULL_HANDLE;
		AllocationVK			m_Allocation;
	};
}
