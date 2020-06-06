#pragma once
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class GraphicsPipelineStateVK : public TDeviceChildBase<GraphicsDeviceVK, PipelineState>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, PipelineState>;

	public:
		GraphicsPipelineStateVK(const GraphicsDeviceVK* pDevice);
		~GraphicsPipelineStateVK();

		bool Init(const GraphicsPipelineStateDesc* pDesc);

		FORCEINLINE VkPipeline GetPipeline() const
		{
			return m_Pipeline;
		}
		
	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

		// PipelineState interface
		virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Pipeline);
		}

		FORCEINLINE virtual EPipelineStateType GetType() const override final
		{
			return EPipelineStateType::PIPELINE_STATE_TYPE_GRAPHICS;
		}

	private:
		void CreateShaderStageInfo(const ShaderModuleDesc* pShaderModule, TArray<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			TArray<VkSpecializationInfo>& shaderStagesSpecializationInfos, TArray<TArray<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps);

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}
