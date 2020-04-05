#pragma once
#include "Rendering/Core/API/IBottomLevelAccelerationStructure.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class BufferVK;

	class BottomLevelAccelerationStructureVK : public DeviceChildBase<GraphicsDeviceVK, IBottomLevelAccelerationStructure>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IBottomLevelAccelerationStructure>;

	public:
		BottomLevelAccelerationStructureVK(const GraphicsDeviceVK* pDevice);
		~BottomLevelAccelerationStructureVK();

		bool Init(const BottomLevelAccelerationStructureDesc& desc);

		virtual void UpdateGeometryData(IBuffer* pVertexBuffer, uint32 firstVertexIndex, IBuffer* pIndexBuffer, uint32 indexBufferByteOffset, uint32 triCount, void* pTransform, IBuffer* pScratchBuffer) override;

		virtual uint64 GetScratchMemorySizeRequirement() override;

		virtual void SetName(const char* pName) override;

	private:
		//INIT
		bool InitAccelerationStructure(const BottomLevelAccelerationStructureDesc& desc);

		//UTIL
		VkMemoryRequirements GetMemoryRequirements(VkAccelerationStructureMemoryRequirementsTypeKHR type);

	private:
		BottomLevelAccelerationStructureDesc m_Desc;
		bool m_AccelerationStructureBuilt;

		VkAccelerationStructureKHR m_AccelerationStructure;
		VkDeviceMemory m_AccelerationStructureMemory;
		VkDeviceAddress m_AccelerationStructureDeviceAddress;

	};
}
