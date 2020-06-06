#pragma once
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class ComputePipelineStateVK : public TDeviceChildBase<GraphicsDeviceVK, PipelineState>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, PipelineState>;

	public:
		ComputePipelineStateVK(const GraphicsDeviceVK* pDevice);
		~ComputePipelineStateVK();

		bool Init(const ComputePipelineStateDesc* pDesc);

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

		virtual EPipelineStateType GetType() const override final
		{
			return EPipelineStateType::PIPELINE_STATE_TYPE_COMPUTE;
		}

	private:
		VkPipeline m_Pipeline;
	};
}
