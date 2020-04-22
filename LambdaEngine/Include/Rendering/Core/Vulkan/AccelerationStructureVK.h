#pragma once
#include "Rendering/Core/API/IAccelerationStructure.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class BufferVK;

	class AccelerationStructureVK : public TDeviceChildBase<GraphicsDeviceVK, IAccelerationStructure>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IAccelerationStructure>;

	public:
		AccelerationStructureVK(const GraphicsDeviceVK* pDevice);
		~AccelerationStructureVK();

		bool Init(const AccelerationStructureDesc* pDesc, IDeviceAllocator* pAllocator);

		FORCEINLINE VkAccelerationStructureKHR GetAccelerationStructure() const
		{
			return m_AccelerationStructure;
		}

		// IDeviceChild interface
		virtual void SetName(const char* pName) override final;

		// IAccelerationStructure interface
		FORCEINLINE virtual uint64 GetScratchMemorySizeRequirement() const override final
		{
			return m_ScratchMemorySize;
		}

		FORCEINLINE virtual uint64 GetScratchMemoryAlignmentRequirement() const override final
		{
			return m_ScratchMemoryAlignment;
		}

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
		VkDeviceSize				m_ScratchMemorySize						= 0;
		VkDeviceSize				m_ScratchMemoryAlignment				= 0;

		AllocationVK				m_Allocation;
		AccelerationStructureDesc	m_Desc;
	};
}
