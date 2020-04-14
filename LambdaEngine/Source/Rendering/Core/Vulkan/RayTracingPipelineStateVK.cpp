#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	RayTracingPipelineStateVK::RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice) :
		TDeviceChild(pDevice),
		m_Pipeline(VK_NULL_HANDLE),
		m_pSBT(nullptr),
		m_BindingStride(0),
		m_BindingOffsetRaygenShaderGroup(0),
		m_BindingOffsetHitShaderGroup(0),
		m_BindingOffsetMissShaderGroup(0)
	{
	}

	RayTracingPipelineStateVK::~RayTracingPipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}

		SAFERELEASE(m_pSBT);
	}

	bool RayTracingPipelineStateVK::Init(const RayTracingPipelineStateDesc& desc)
	{
		// Define shader stage create infos
		std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos;
		std::vector<VkSpecializationInfo> shaderStagesSpecializationInfos;
		std::vector<std::vector<VkSpecializationMapEntry>> shaderStagesSpecializationMaps;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

		if (!CreateShaderData(shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps, shaderGroups, desc))
        {
            return false;
        }

		VkPipelineLibraryCreateInfoKHR rayTracingPipelineLibrariesInfo = {};
		rayTracingPipelineLibrariesInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		rayTracingPipelineLibrariesInfo.libraryCount	= 0;
		rayTracingPipelineLibrariesInfo.pLibraries		= nullptr;

		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo = {};
		rayTracingPipelineInfo.sType				= VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineInfo.stageCount			= (uint32)shaderStagesInfos.size();
		rayTracingPipelineInfo.pStages				= shaderStagesInfos.data();
		rayTracingPipelineInfo.groupCount			= (uint32)shaderGroups.size();
		rayTracingPipelineInfo.pGroups				= shaderGroups.data();
		rayTracingPipelineInfo.maxRecursionDepth	= desc.MaxRecursionDepth;
		rayTracingPipelineInfo.layout				= reinterpret_cast<const PipelineLayoutVK*>(desc.pPipelineLayout)->GetPipelineLayout();
		rayTracingPipelineInfo.libraries			= rayTracingPipelineLibrariesInfo;

		if (m_pDevice->vkCreateRayTracingPipelinesKHR(m_pDevice->Device, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		{
			LOG_ERROR("[RayTracingPipelineStateVK]: vkCreateRayTracingPipelinesKHR failed for \"%s\"", desc.pName);
			return false;
		}

		uint32 shaderGroupHandleSize = m_pDevice->RayTracingProperties.shaderGroupHandleSize;
		uint32 sbtSize = shaderGroupHandleSize * shaderGroups.size();;

		BufferDesc sbtBufferDesc = {};
		sbtBufferDesc.pName			= "Shader Binding Table";
		sbtBufferDesc.Flags			= BUFFER_FLAG_RAY_TRACING;
		sbtBufferDesc.MemoryType	= EMemoryType::MEMORY_CPU_VISIBLE;
		sbtBufferDesc.SizeInBytes = sbtSize;

		m_pSBT = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(sbtBufferDesc));

		void* pMapped = m_pSBT->Map();
		if (m_pDevice->vkGetRayTracingShaderGroupHandlesKHR(m_pDevice->Device, m_Pipeline, 0, shaderGroups.size(), sbtSize, pMapped) != VK_SUCCESS)
		{
			LOG_ERROR("[RayTracingPipelineStateVK]: vkGetRayTracingShaderGroupHandlesKHR failed for \"%s\"", desc.pName);
			return false;
		}
		m_pSBT->Unmap();

		m_BindingStride						= shaderGroupHandleSize;
		m_BindingOffsetRaygenShaderGroup	= 0;
		m_BindingOffsetHitShaderGroup		= m_BindingOffsetRaygenShaderGroup + m_BindingStride;
		m_BindingOffsetMissShaderGroup		= m_BindingOffsetHitShaderGroup + m_BindingStride * desc.ClosestHitShaderCount;

		SetName(desc.pName);

		return true;
	}

	void RayTracingPipelineStateVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Pipeline, VK_OBJECT_TYPE_PIPELINE);
		}
	}

	bool RayTracingPipelineStateVK::CreateShaderData(
		std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos, 
		std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos, 
		std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps, 
		std::vector<VkRayTracingShaderGroupCreateInfoKHR>& shaderGroups,
		const RayTracingPipelineStateDesc& desc)
	{
		//Raygen Shader
		{
			const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(desc.pRaygenShader);

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

			pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
			pShader->FillShaderStageInfo(shaderCreateInfo, shaderSpecializationInfo);

			shaderStagesInfos.push_back(shaderCreateInfo);
			shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
			shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader			= 0;
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= VK_SHADER_UNUSED_NV;
			shaderGroups.push_back(shaderGroupCreateInfo);
		}

		//Closest-Hit Shaders
		for (uint32 i = 0; i < desc.ClosestHitShaderCount; i++)
		{
			const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(desc.ppClosestHitShaders[i]);

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

			pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
			pShader->FillShaderStageInfo(shaderCreateInfo, shaderSpecializationInfo);

			shaderStagesInfos.push_back(shaderCreateInfo);
			shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
			shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroupCreateInfo.generalShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= uint32(shaderStagesInfos.size() - 1);
			shaderGroups.push_back(shaderGroupCreateInfo);
		}

		//Miss Shaders
		for (uint32 i = 0; i < desc.MissShaderCount; i++)
		{
			const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(desc.ppMissShaders[i]);

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

			pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
			pShader->FillShaderStageInfo(shaderCreateInfo, shaderSpecializationInfo);

			shaderStagesInfos.push_back(shaderCreateInfo);
			shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
			shaderStagesSpecializationMaps.push_back(shaderSpecializationMapEntries);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader			= uint32(shaderStagesInfos.size() - 1);
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= VK_SHADER_UNUSED_NV;
			shaderGroups.push_back(shaderGroupCreateInfo);
		}

		return true;
	}
}
