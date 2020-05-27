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

	bool ComputePipelineStateVK::Init(const ComputePipelineStateDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		const ShaderVK*			pShader				= reinterpret_cast<const ShaderVK*>(pDesc->pShader);
		const PipelineLayoutVK* pPipelineLayoutVk	= reinterpret_cast<const PipelineLayoutVK*>(pDesc->pPipelineLayout);
		VALIDATE(pShader			!= nullptr);
		VALIDATE(pPipelineLayoutVk	!= nullptr);

		// ShaderStageInfo
		VkPipelineShaderStageCreateInfo shaderCreateInfo = { };
		shaderCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.pNext	= nullptr;
		shaderCreateInfo.flags	= 0;
		shaderCreateInfo.stage	= VK_SHADER_STAGE_COMPUTE_BIT;
		shaderCreateInfo.module	= pShader->GetShaderModule();
		shaderCreateInfo.pName	= pShader->GetEntryPoint();

		// Shader Constants
		VkSpecializationInfo specializationInfo = { };
		if (pDesc->pShader->ShaderConstantCount)
		{
			TArray<VkSpecializationMapEntry> specializationEntires(pDesc->pShader->ShaderConstantCount);
			for (uint32 i = 0; i < pDesc->pShader->ShaderConstantCount; i++)
			{
				VkSpecializationMapEntry specializationEntry = {};
				specializationEntry.constantID	= i;
				specializationEntry.offset		= i * sizeof(ShaderConstant);
				specializationEntry.size		= sizeof(ShaderConstant);
				specializationEntires.emplace_back(specializationEntry);
			}

			specializationInfo.mapEntryCount = static_cast<uint32>(specializationEntires.size());
			specializationInfo.pMapEntries	 = specializationEntires.data();
			specializationInfo.dataSize		 = pDesc->pShader->ShaderConstantCount * sizeof(ShaderConstant);
			specializationInfo.pData		 = pDesc->pShader->pConstants;

			shaderCreateInfo.pSpecializationInfo = &specializationInfo;
		}
		else
		{
			shaderCreateInfo.pSpecializationInfo = nullptr;
		}

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType					= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext					= nullptr;
		pipelineInfo.flags					= 0;
		pipelineInfo.layout					= pPipelineLayoutVk->GetPipelineLayout();
		pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex		= -1;
		pipelineInfo.stage					= shaderCreateInfo;

		VkResult result = vkCreateComputePipelines(m_pDevice->Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
			if (pDesc->pName)
			{
				LOG_VULKAN_ERROR(result, "[ComputePipelineStateVK]: vkCreateComputePipelines failed for %s", pDesc->pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[ComputePipelineStateVK]: vkCreateComputePipelines failed");
			}
			
			return false;
		}
		else
		{
			SetName(pDesc->pName);

			if (pDesc->pName)
			{
				D_LOG_MESSAGE("[ComputePipelineStateVK]: Created Pipeline for %s", pDesc->pName);
			}
			else
			{
				D_LOG_MESSAGE("[ComputePipelineStateVK]: Created Pipeline");
			}
			
			return true;
		}
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
