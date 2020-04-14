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

		bool Init(const GraphicsPipelineStateDesc& desc);

        FORCEINLINE VkPipeline GetPipeline() const
        {
            return m_Pipeline;
        }
        
        // IDeviceChild interface
		virtual void SetName(const char* pName) override final;

        // IPipelineState interface
		FORCEINLINE virtual EPipelineStateType GetType() const override final
        {
            return EPipelineStateType::GRAPHICS;
        }

	private:
		bool CreateShaderData(std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos,
			std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps,
			const GraphicsPipelineStateDesc& desc);

	private:
		VkPipeline m_Pipeline;

		VkPipelineInputAssemblyStateCreateInfo	m_InputAssembly;
		VkPipelineRasterizationStateCreateInfo	m_RasterizerState;
		VkPipelineMultisampleStateCreateInfo	m_MultisamplingState;
		VkPipelineColorBlendStateCreateInfo		m_BlendState;
		VkPipelineDepthStencilStateCreateInfo	m_DepthStencilState;

		VkPipelineColorBlendAttachmentState*	m_pColorBlendAttachmentStates;
	};
}
