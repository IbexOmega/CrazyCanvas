#pragma once
#include "Core/Ref.h"

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
		
		bool Init(const BufferDesc* pDesc, DeviceAllocator* pAllocator);
		
		FORCEINLINE VkBuffer GetBuffer() const
		{
			return m_Buffer;
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// Buffer interface
		virtual void*	Map()	override final;
		virtual void	Unmap()	override final;
		
		virtual uint64 GetDeviceAdress()			const override final;
		virtual uint64 GetAlignmentRequirement()	const override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Buffer);
		}

	private:
		Ref<DeviceAllocatorVK>  m_Allocator					= nullptr;
		VkBuffer				m_Buffer					= VK_NULL_HANDLE;
		VkDeviceMemory			m_Memory					= VK_NULL_HANDLE;
		VkDeviceAddress			m_DeviceAddress				= 0;
		VkDeviceAddress			m_AlignementRequirement		= 0;
		bool					m_IsMapped					= false;
		AllocationVK			m_Allocation;
	};
}
