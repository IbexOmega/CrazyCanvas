#pragma once
#include "Core/Ref.h"

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

		bool Init(CommandQueue* pCommandQueue, const RayTracingPipelineStateDesc* pDesc);

		FORCEINLINE VkDeviceSize GetBindingOffsetRaygenGroup() const 
		{ 
			return m_BindingOffsetRaygenShaderGroup; 
		}
		
		FORCEINLINE VkDeviceSize GetBindingOffsetHitGroup() const 
		{ 
			return m_BindingOffsetHitShaderGroup; 
		}
		
		FORCEINLINE VkDeviceSize GetBindingOffsetMissGroup() const 
		{ 
			return m_BindingOffsetMissShaderGroup; 
		}
		
		FORCEINLINE VkDeviceSize GetBindingSizeRaygenGroup() const 
		{ 
			return m_BindingSizeRaygenShaderGroup;
		}

		FORCEINLINE VkDeviceSize GetBindingSizeHitGroup() const 
		{ 
			return m_BindingSizeHitShaderGroup; 
		}

		FORCEINLINE VkDeviceSize GetBindingSizeMissGroup() const 
		{ 
			return m_BindingSizeMissShaderGroup;
		}

		FORCEINLINE VkDeviceSize GetBindingStride() const
		{ 
			return m_BindingStride;
		}

		FORCEINLINE VkPipeline GetPipeline() const
		{
			return m_Pipeline;
		}
		
		FORCEINLINE const BufferVK* GetShaderBindingTable() const
		{
			return m_ShaderBindingTable.Get();
		}
		
	public:
		//DeviceChild interface
		virtual void SetName(const String& name) override final;

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
		VkPipeline		m_Pipeline						= VK_NULL_HANDLE;
		Ref<BufferVK>	m_ShaderHandleStorageBuffer		= nullptr;
		Ref<BufferVK>	m_ShaderBindingTable			= nullptr;

		VkDeviceSize m_BindingOffsetRaygenShaderGroup	= 0;
		VkDeviceSize m_BindingOffsetHitShaderGroup		= 0;
		VkDeviceSize m_BindingOffsetMissShaderGroup		= 0;
		VkDeviceSize m_BindingSizeRaygenShaderGroup		= 0;
		VkDeviceSize m_BindingSizeHitShaderGroup		= 0;
		VkDeviceSize m_BindingSizeMissShaderGroup		= 0;
		VkDeviceSize m_BindingStride					= 0;
	};
}
