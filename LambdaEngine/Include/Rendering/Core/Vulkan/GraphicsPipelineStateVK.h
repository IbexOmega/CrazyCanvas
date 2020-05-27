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
			return EPipelineStateType::PIPELINE_TYPE_GRAPHICS;
		}

	private:
		void CreateShaderStageInfo(const ShaderModuleDesc* pShaderModule, TArray<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			TArray<VkSpecializationInfo>& shaderStagesSpecializationInfos, TArray<TArray<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps);

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}
