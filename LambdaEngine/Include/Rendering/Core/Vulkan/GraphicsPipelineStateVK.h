#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class GraphicsPipelineStateVK : public DeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		GraphicsPipelineStateVK(const GraphicsDeviceVK* pDevice);
		~GraphicsPipelineStateVK();

		bool Init(const GraphicsPipelineDesc& desc);

		virtual void SetName(const char* pName) override;

		virtual EPipelineStateType GetType() override { return EPipelineStateType::GRAPHICS; }

	private:
		bool CreateShaderData(std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos,
			std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps,
			const GraphicsPipelineDesc& desc);

	private:
		const GraphicsDeviceVK* m_pDevice;

		VkPipeline m_Pipeline;

		VkPipelineInputAssemblyStateCreateInfo	m_InputAssembly;
		VkPipelineRasterizationStateCreateInfo	m_RasterizerState;
		VkPipelineMultisampleStateCreateInfo	m_MultisamplingState;
		VkPipelineColorBlendStateCreateInfo		m_BlendState;
		VkPipelineDepthStencilStateCreateInfo	m_DepthStencilState;
	};
}