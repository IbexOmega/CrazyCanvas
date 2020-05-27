#pragma once
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class ICommandAllocator;
	class GraphicsDeviceVK;
	class ICommandList;
	class BufferVK;

	class RayTracingPipelineStateVK : public TDeviceChildBase<GraphicsDeviceVK, IPipelineState>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IPipelineState>;

	public:
		RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice);
		~RayTracingPipelineStateVK();

		bool Init(const RayTracingPipelineStateDesc* pDesc);

		FORCEINLINE VkDeviceSize GetBindingOffsetRaygenGroup()	const { return m_BindingOffsetRaygenShaderGroup; }
		FORCEINLINE VkDeviceSize GetBindingOffsetHitGroup()		const { return m_BindingOffsetHitShaderGroup; }
		FORCEINLINE VkDeviceSize GetBindingOffsetMissGroup()	const { return m_BindingOffsetMissShaderGroup; }
        FORCEINLINE VkDeviceSize GetBindingSizeRaygenGroup()    const { return m_BindingSizeRaygenShaderGroup; }
        FORCEINLINE VkDeviceSize GetBindingSizeHitGroup()       const { return m_BindingSizeHitShaderGroup; }
        FORCEINLINE VkDeviceSize GetBindingSizeMissGroup()      const { return m_BindingSizeMissShaderGroup; }
		FORCEINLINE VkDeviceSize GetBindingStride()				const { return m_BindingStride; }

		FORCEINLINE VkPipeline GetPipeline() const
        {
            return m_Pipeline;
        }
        
        FORCEINLINE BufferVK* GetShaderBindingTable() const
        {
            return m_pSBT;
        }
        
        //IDeviceChild interface
		virtual void SetName(const char* pName) override final;

        //IPipelineState interface
		FORCEINLINE virtual EPipelineStateType GetType() const override final
        {
            return EPipelineStateType::PIPELINE_RAY_TRACING;
        }

	private:
		bool CreateShaderData(std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos,
			std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps,
			std::vector<VkRayTracingShaderGroupCreateInfoKHR>& shaderGroups,
			const RayTracingPipelineStateDesc* pDesc);

	private:
		VkPipeline	m_Pipeline						= VK_NULL_HANDLE;
		BufferVK*	m_pShaderHandleStorageBuffer	= nullptr;
        BufferVK*	m_pSBT							= nullptr;

		ICommandAllocator*	m_pCommandAllocator		= nullptr;
		ICommandList*		m_pCommandList			= nullptr;

		VkDeviceSize m_BindingOffsetRaygenShaderGroup   = 0;
		VkDeviceSize m_BindingOffsetHitShaderGroup      = 0;
		VkDeviceSize m_BindingOffsetMissShaderGroup     = 0;
        VkDeviceSize m_BindingSizeRaygenShaderGroup     = 0;
        VkDeviceSize m_BindingSizeHitShaderGroup        = 0;
        VkDeviceSize m_BindingSizeMissShaderGroup       = 0;
		VkDeviceSize m_BindingStride                    = 0;
	};
}
