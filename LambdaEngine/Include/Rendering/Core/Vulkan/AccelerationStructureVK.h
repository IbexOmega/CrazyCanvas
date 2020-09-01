#pragma once
#include "Core/Ref.h"

#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"

namespace LambdaEngine
{
	class BufferVK;
	class GraphicsDeviceVK;

	class AccelerationStructureVK : public TDeviceChildBase<GraphicsDeviceVK, AccelerationStructure>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, AccelerationStructure>;

	public:
		AccelerationStructureVK(const GraphicsDeviceVK* pDevice);
		~AccelerationStructureVK();

		bool Init(const AccelerationStructureDesc* pDesc, DeviceAllocator* pAllocator);

		FORCEINLINE VkAccelerationStructureKHR GetAccelerationStructure() const
		{
			return m_AccelerationStructure;
		}

		FORCEINLINE BufferVK* GetScratchBuffer()
		{
			return m_ScratchBuffer.Get();
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// AccelerationStructure interface
		FORCEINLINE virtual uint64 GetDeviceAdress() const override final
		{
			return m_AccelerationStructureDeviceAddress;
		}

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_AccelerationStructure);
		}

	private:
		VkMemoryRequirements GetMemoryRequirements(VkAccelerationStructureMemoryRequirementsTypeKHR type);

	private:
		TSharedRef<DeviceAllocatorVK>		m_Allocator								= nullptr;
		VkAccelerationStructureKHR	m_AccelerationStructure					= VK_NULL_HANDLE;
		VkDeviceMemory				m_AccelerationStructureMemory			= VK_NULL_HANDLE;
		VkDeviceAddress				m_AccelerationStructureDeviceAddress	= 0;
		TSharedRef<BufferVK>				m_ScratchBuffer							= nullptr;
		AllocationVK				m_Allocation;
	};
}
