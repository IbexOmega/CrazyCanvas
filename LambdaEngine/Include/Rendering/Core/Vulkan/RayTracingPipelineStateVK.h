#pragma once
#include "Core/TSharedRef.h"

#include "Rendering/Core/API/PipelineState.h"

#include "Containers/TArray.h"

#include "BufferVK.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class CommandQueue;
	class GraphicsDeviceVK;

	class RayTracingPipelineStateVK : public TDeviceChildBase<GraphicsDeviceVK, PipelineState>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, PipelineState>;

	public:
		RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice);
		~RayTracingPipelineStateVK();

		bool Init(const RayTracingPipelineStateDesc* pDesc);

		FORCEINLINE VkPipeline GetPipeline() const
		{
			return m_Pipeline;
		}
		
		FORCEINLINE virtual uint32 HitShaderCount() const { return m_HitShaderCount; }
		FORCEINLINE virtual uint32 MissShaderCount() const { return m_MissShaderCount; }

		//DeviceChild interface
		virtual void SetName(const String& debugName) override final;

		//IPipelineState interface
		virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_Pipeline);
		}

		virtual EPipelineStateType GetType() const override final
		{
			return EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING;
		}

	private:
		void CreateShaderStageInfo(const ShaderModuleDesc* pShaderModule, TArray<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			TArray<VkSpecializationInfo>& shaderStagesSpecializationInfos, TArray<TArray<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps);

	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

		uint32 m_HitShaderCount		= 0;
		uint32 m_MissShaderCount	= 0;
	};
}
