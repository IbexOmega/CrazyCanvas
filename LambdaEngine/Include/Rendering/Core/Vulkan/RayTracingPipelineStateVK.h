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

		FORCEINLINE VkPipeline GetPipeline() const
        {
            return m_Pipeline;
        }

		FORCEINLINE BufferVK* GetSBT() const
		{
			return m_pSBT;
		}
        
        FORCEINLINE const VkStridedBufferRegionKHR* GetRaygenBufferRegion() const
        {
            return &m_RaygenBufferRegion;
        }

		FORCEINLINE const VkStridedBufferRegionKHR* GetHitBufferRegion() const
		{
			return &m_HitBufferRegion;
		}

		FORCEINLINE const VkStridedBufferRegionKHR* GetMissBufferRegion() const
		{
			return &m_MissBufferRegion;
		}

		FORCEINLINE const VkStridedBufferRegionKHR* GetCallableBufferRegion() const
		{
			return &m_CallableBufferRegion;
		}
        
        //IDeviceChild interface
		virtual void SetName(const char* pName) override final;

        //IPipelineState interface
		FORCEINLINE virtual EPipelineStateType GetType() const override final
        {
            return EPipelineStateType::RAY_TRACING;
        }

	private:
		bool CreateShaderData(std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
			std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos,
			std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps,
			std::vector<VkRayTracingShaderGroupCreateInfoKHR>& shaderGroups,
			const RayTracingPipelineStateDesc* pDesc);

	private:
		VkPipeline	m_Pipeline								= VK_NULL_HANDLE;
		BufferVK*	m_pShaderHandleStorageBuffer			= nullptr;
        BufferVK*	m_pSBT									= nullptr;

		ICommandAllocator*	m_pCommandAllocator				= nullptr;
		ICommandList*		m_pCommandList					= nullptr;

		VkStridedBufferRegionKHR	m_RaygenBufferRegion	= {};
		VkStridedBufferRegionKHR	m_HitBufferRegion		= {};
		VkStridedBufferRegionKHR	m_MissBufferRegion		= {};
		VkStridedBufferRegionKHR	m_CallableBufferRegion	= {};
	};
}
