#pragma once
#include "Rendering/Core/API/ITopLevelAccelerationStructure.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class BufferVK;

	class TopLevelAccelerationStructureVK : public TDeviceChildBase<GraphicsDeviceVK, ITopLevelAccelerationStructure>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, ITopLevelAccelerationStructure>;

	public:
		TopLevelAccelerationStructureVK(const GraphicsDeviceVK* pDevice);
		~TopLevelAccelerationStructureVK();

		bool Init(const TopLevelAccelerationStructureDesc& desc);

		FORCEINLINE VkAccelerationStructureKHR GetAccelerationStructure() const
		{
			return m_AccelerationStructure;
		}

		// IDeviceChild interface
		virtual void SetName(const char* pName) override final;
		
		// ITopLevelAccelerationStructure interface
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

		FORCEINLINE virtual TopLevelAccelerationStructureDesc GetDesc() const override final
		{
			return m_Desc;
		}

	private:
		VkMemoryRequirements GetMemoryRequirements(VkAccelerationStructureMemoryRequirementsTypeKHR type);

	private:
		VkAccelerationStructureKHR	m_AccelerationStructure					= VK_NULL_HANDLE;
		VkDeviceMemory				m_AccelerationStructureMemory			= VK_NULL_HANDLE;
		VkDeviceAddress				m_AccelerationStructureDeviceAddress	= 0;
		VkDeviceSize				m_ScratchMemorySize						= 0;
		VkDeviceSize				m_ScratchMemoryAlignment				= 0;

		TopLevelAccelerationStructureDesc m_Desc;
	};
}
