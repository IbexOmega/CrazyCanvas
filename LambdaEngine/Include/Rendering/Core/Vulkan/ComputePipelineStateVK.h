#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class ComputePipelineStateVK : public DeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		ComputePipelineStateVK(const GraphicsDeviceVK* pDevice);
		~ComputePipelineStateVK();

		bool Init(const ComputePipelineDesc& desc);

		virtual void SetName(const char* pName) override;

		virtual EPipelineStateType GetType() override { return EPipelineStateType::COMPUTE; }

	private:
		const GraphicsDeviceVK* m_pDevice;

		VkPipeline m_Pipeline;
	};
}