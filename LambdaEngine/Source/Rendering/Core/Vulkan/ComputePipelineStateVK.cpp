#include "Rendering/Core/Vulkan/ComputePipelineStateVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	ComputePipelineStateVK::ComputePipelineStateVK(const GraphicsDeviceVK* pDevice) 
		: TDeviceChild(pDevice),
		m_Pipeline(VK_NULL_HANDLE)
	{
	}

	ComputePipelineStateVK::~ComputePipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}
	}

	bool ComputePipelineStateVK::Init(const ComputePipelineStateDesc& desc)
	{
		const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(desc.pShader);

		VkPipelineShaderStageCreateInfo shaderCreateInfo;
		VkSpecializationInfo shaderSpecializationInfo;
		std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

		pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
		pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType					= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext					= nullptr;
		pipelineInfo.flags					= 0;
		pipelineInfo.layout					= reinterpret_cast<const PipelineLayoutVK*>(desc.pPipelineLayout)->GetPipelineLayout();;
		pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex		= -1;
		pipelineInfo.stage					= shaderCreateInfo;

		if (vkCreateComputePipelines(m_pDevice->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
		{
			LOG_ERROR("[ComputePipelineStateVK]: vkCreateComputePipelines failed for %s", desc.pName);
			return false;
		}

		SetName(desc.pName);

		return true;
	}

	void ComputePipelineStateVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_Pipeline, VK_OBJECT_TYPE_PIPELINE);
		}
	}
}