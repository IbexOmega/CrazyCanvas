#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	RayTracingPipelineStateVK::RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice) :
		TDeviceChild(pDevice),
		m_Pipeline(VK_NULL_HANDLE),
		m_pSBT(nullptr)
	{
	}

	RayTracingPipelineStateVK::~RayTracingPipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}

		SAFEDELETE(m_pSBT);
	}

	bool RayTracingPipelineStateVK::Init(const RayTracingPipelineDesc& desc)
	{
		// Define shader stage create infos
		std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos;
		std::vector<VkSpecializationInfo> shaderStagesSpecializationInfos;
		std::vector<std::vector<VkSpecializationMapEntry>> shaderStagesSpecializationMaps;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

		if (!CreateShaderData(shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps, shaderGroups, desc))
			return false;

		VkRayTracingPipelineCreateInfoKHR rayPipelineInfo = {};
		rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayPipelineInfo.stageCount = (uint32)shaderStagesInfos.size();
		rayPipelineInfo.pStages = shaderStagesInfos.data();
		rayPipelineInfo.groupCount = (uint32)shaderGroups.size();
		rayPipelineInfo.pGroups = shaderGroups.data();
		rayPipelineInfo.maxRecursionDepth = desc.MaxRecursionDepth;
		rayPipelineInfo.layout = VK_NULL_HANDLE;

		if (m_pDevice->vkCreateRayTracingPipelinesKHR(m_pDevice->Device, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		{
			LOG_ERROR("--- RayTracingPipelineStateVK: vkCreateRayTracingPipelinesKHR failed for %s", desc.pName);
			return false;
		}

		SetName(desc.pName);

		return true;
	}

	void RayTracingPipelineStateVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_Pipeline, VK_OBJECT_TYPE_PIPELINE);
	}

	bool RayTracingPipelineStateVK::CreateShaderData(
		std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos, 
		std::vector<VkSpecializationInfo>& shaderStagesSpecializationInfos, 
		std::vector<std::vector<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps, 
		std::vector<VkRayTracingShaderGroupCreateInfoKHR>& shaderGroups,
		const RayTracingPipelineDesc& desc)
	{
		//Raygen Shader
		{
			const ShaderDesc& shaderDesc = desc.RaygenShader;

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

			CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

			if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
				return false;

			shaderStagesInfos.push_back(shaderCreateInfo);
			shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
			shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader = 0;
			shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_NV;
			shaderGroups.push_back(shaderGroupCreateInfo);
		}

		//Closest-Hit Shaders
		for (const ShaderDesc& shaderDesc : desc.ClosestHitShaders)
		{
			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

			CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

			if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
				return false;

			shaderStagesInfos.push_back(shaderCreateInfo);
			shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
			shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader = shaderStagesInfos.size() - 1;
			shaderGroups.push_back(shaderGroupCreateInfo);
		}

		//Miss Shaders
		for (const ShaderDesc& shaderDesc : desc.MissShaders)
		{
			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMaps;

			CreateSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMaps, shaderDesc);

			if (!CreateShaderStageInfo(m_pDevice->Device, shaderCreateInfo, shaderDesc, shaderSpecializationInfo))
				return false;

			shaderStagesInfos.push_back(shaderCreateInfo);
			shaderStagesSpecializationInfos.push_back(shaderSpecializationInfo);
			shaderStagesSpecializationMaps.push_back(shaderSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader = shaderStagesInfos.size() - 1;
			shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_NV;
			shaderGroups.push_back(shaderGroupCreateInfo);
		}

		return true;
	}
}