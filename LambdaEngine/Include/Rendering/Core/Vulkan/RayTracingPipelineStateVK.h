#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class BufferVK;

	class RayTracingPipelineStateVK : public DeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice);
		~RayTracingPipelineStateVK();

		bool Init(const RayTracingPipelineDesc& desc);

		virtual void SetName(const char* pName) override;

		virtual EPipelineStateType GetType() override { return EPipelineStateType::RAY_TRACING; }

	private:
		bool CreateShaderData(std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos,
			std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps,
			std::vector<VkRayTracingShaderGroupCreateInfoKHR>& shaderGroups,
			const RayTracingPipelineDesc& desc);

	private:
		const GraphicsDeviceVK* m_pDevice;

		VkPipeline m_Pipeline;
		BufferVK* m_pSBT;

		VkDeviceSize m_BindingOffsetRaygenShaderGroup;
		VkDeviceSize m_BindingOffsetHitShaderGroup;
		VkDeviceSize m_BindingOffsetMissShaderGroup;
		VkDeviceSize m_BindingStride;
	};
}