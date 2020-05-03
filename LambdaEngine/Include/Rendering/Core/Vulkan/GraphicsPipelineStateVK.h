#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class GraphicsPipelineStateVK : public TDeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		GraphicsPipelineStateVK(const GraphicsDeviceVK* pDevice);
		~GraphicsPipelineStateVK();

		bool Init(const GraphicsPipelineStateDesc* pDesc);

        FORCEINLINE VkPipeline GetPipeline() const
        {
            return m_Pipeline;
        }
        
        // IDeviceChild interface
		virtual void SetName(const char* pName) override final;

        // IPipelineState interface
		FORCEINLINE virtual EPipelineStateType GetType() const override final
        {
            return EPipelineStateType::PIPELINE_GRAPHICS;
        }

	private:
		bool CreateShaderData(std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos,
			std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps,
			const GraphicsPipelineStateDesc* pDesc);

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}
