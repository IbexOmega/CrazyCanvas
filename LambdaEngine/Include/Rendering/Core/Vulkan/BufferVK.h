#pragma once
#include "Core/TSharedRef.h"

#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class BufferVK : public TDeviceChildBase<GraphicsDeviceVK, Buffer>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, Buffer>;

	public:
		BufferVK(const GraphicsDeviceVK* pDevice);
		~BufferVK();
		
		bool Init(const BufferDesc* pDesc);
		
		FORCEINLINE VkBuffer GetBuffer() const
		{
			return m_Buffer;
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// Buffer interface
		virtual void* Map() override final;
		virtual void Unmap() override final;
		
		virtual uint64 GetDeviceAddress() const override final;
		virtual uint64 GetAlignmentRequirement() const override final;

		inline virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Buffer);
		}

	private:
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		AllocationVK m_Allocation;
		
		VkDeviceAddress	m_DeviceAddress = 0;
		VkDeviceAddress	m_AlignmentRequirement = 0;
		
		bool m_IsMapped = false;
	};
}
