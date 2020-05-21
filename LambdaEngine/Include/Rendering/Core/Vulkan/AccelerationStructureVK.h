#pragma once
#include "Rendering/Core/API/IAccelerationStructure.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"

namespace LambdaEngine
{
	class BufferVK;
	class GraphicsDeviceVK;

	class AccelerationStructureVK : public TDeviceChildBase<GraphicsDeviceVK, IAccelerationStructure>
	{
		friend class RayTracingTestVK;

		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IAccelerationStructure>;

	public:
		AccelerationStructureVK(const GraphicsDeviceVK* pDevice);
		~AccelerationStructureVK();

		bool Init(const AccelerationStructureDesc* pDesc, IDeviceAllocator* pAllocator);

		FORCEINLINE VkAccelerationStructureKHR GetAccelerationStructure() const
		{
			return m_AccelerationStructure;
		}

		FORCEINLINE BufferVK* GetScratchBuffer()
		{
			return m_pScratchBuffer;
		}

		// IDeviceChild interface
		virtual void SetName(const char* pName) override final;

		FORCEINLINE virtual uint64 GetDeviceAdress() const override final
		{
			return uint64(m_AccelerationStructureDeviceAddress);
		}

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return uint64(m_AccelerationStructure);
		}

		FORCEINLINE virtual AccelerationStructureDesc GetDesc() const override final
		{
			return m_Desc;
		}

	private:
		VkMemoryRequirements GetMemoryRequirements(VkAccelerationStructureMemoryRequirementsTypeKHR type);

	private:
		DeviceAllocatorVK*			m_pAllocator							= nullptr;
		VkAccelerationStructureKHR	m_AccelerationStructure					= VK_NULL_HANDLE;
		VkDeviceMemory				m_AccelerationStructureMemory			= VK_NULL_HANDLE;
		VkDeviceAddress				m_AccelerationStructureDeviceAddress	= 0;
		BufferVK*					m_pScratchBuffer						= nullptr;

		AllocationVK				m_Allocation;
		AccelerationStructureDesc	m_Desc;
	};
}
