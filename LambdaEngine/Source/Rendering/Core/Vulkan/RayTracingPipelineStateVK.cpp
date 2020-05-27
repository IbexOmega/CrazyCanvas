#include "Log/Log.h"

#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"

namespace LambdaEngine
{
	RayTracingPipelineStateVK::RayTracingPipelineStateVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	RayTracingPipelineStateVK::~RayTracingPipelineStateVK()
	{
		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_pDevice->Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}

		SAFERELEASE(m_pShaderBindingTable);
		SAFERELEASE(m_pShaderHandleStorageBuffer);
	}

	bool RayTracingPipelineStateVK::Init(ICommandQueue* pCommandQueue, const RayTracingPipelineStateDesc* pDesc)
	{
		// Define shader stage create infos
		TArray<VkPipelineShaderStageCreateInfo>			shaderStagesInfos;
		TArray<VkSpecializationInfo>					shaderStagesSpecializationInfos;
		TArray<TArray<VkSpecializationMapEntry>>		shaderStagesSpecializationMaps;
		TArray<VkRayTracingShaderGroupCreateInfoKHR>	shaderGroups;

		//Raygen Shader
		{
			CreateShaderStageInfo(pDesc->pRaygenShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader			= 0;
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= VK_SHADER_UNUSED_NV;
			shaderGroups.emplace_back(shaderGroupCreateInfo);
		}

		//Closest-Hit Shaders
		for (uint32 i = 0; i < pDesc->ClosestHitShaderCount; i++)
		{
			const ShaderModuleDesc* pShaderModuleDesc = pDesc->ppClosestHitShaders[i];
			CreateShaderStageInfo(pDesc->pRaygenShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroupCreateInfo.generalShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= static_cast<uint32>(shaderStagesInfos.size() - 1);
			shaderGroups.emplace_back(shaderGroupCreateInfo);
		}

		//Miss Shaders
		for (uint32 i = 0; i < pDesc->MissShaderCount; i++)
		{
			const ShaderModuleDesc* pShaderModuleDesc = pDesc->ppClosestHitShaders[i];
			CreateShaderStageInfo(pDesc->pRaygenShader, shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps);

			VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo = {};
			shaderGroupCreateInfo.sType					= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroupCreateInfo.type					= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroupCreateInfo.generalShader			= static_cast<uint32>(shaderStagesInfos.size() - 1);
			shaderGroupCreateInfo.intersectionShader	= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.anyHitShader			= VK_SHADER_UNUSED_NV;
			shaderGroupCreateInfo.closestHitShader		= VK_SHADER_UNUSED_NV;
			shaderGroups.emplace_back(shaderGroupCreateInfo);
		}

		VkPipelineLibraryCreateInfoKHR rayTracingPipelineLibrariesInfo = {};
		rayTracingPipelineLibrariesInfo.sType		 = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		rayTracingPipelineLibrariesInfo.libraryCount = 0;
		rayTracingPipelineLibrariesInfo.pLibraries	 = nullptr;

		const PipelineLayoutVK* pPipelineLayoutVk = reinterpret_cast<const PipelineLayoutVK*>(pDesc->pPipelineLayout);
		
		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo = {};
		rayTracingPipelineInfo.sType			 = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineInfo.flags			 = VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR | VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR;
		rayTracingPipelineInfo.stageCount		 = static_cast<uint32>(shaderStagesInfos.size());
		rayTracingPipelineInfo.pStages			 = shaderStagesInfos.data();
		rayTracingPipelineInfo.groupCount		 = static_cast<uint32>(shaderGroups.size());
		rayTracingPipelineInfo.pGroups			 = shaderGroups.data();
		rayTracingPipelineInfo.maxRecursionDepth = pDesc->MaxRecursionDepth;
		rayTracingPipelineInfo.layout			 = pPipelineLayoutVk->GetPipelineLayout();
		rayTracingPipelineInfo.libraries		 = rayTracingPipelineLibrariesInfo;

		VkResult result = m_pDevice->vkCreateRayTracingPipelinesKHR(m_pDevice->Device, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
			if (pDesc->pName)
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkCreateRayTracingPipelinesKHR failed for \"%s\"", pDesc->pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkCreateRayTracingPipelinesKHR failed");
			}
			
			return false;
		}

		const uint32 shaderGroupHandleSize	= m_pDevice->RayTracingProperties.shaderGroupHandleSize;
		const uint32 sbtSize				= shaderGroupHandleSize * uint32(shaderGroups.size());

		BufferDesc shaderHandleStorageDesc = {};
		shaderHandleStorageDesc.pName		= "Shader Handle Storage";
		shaderHandleStorageDesc.Flags		= BUFFER_FLAG_COPY_SRC;
		shaderHandleStorageDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		shaderHandleStorageDesc.SizeInBytes	= sbtSize;

		m_pShaderHandleStorageBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&shaderHandleStorageDesc, nullptr));

		void* pMapped = m_pShaderHandleStorageBuffer->Map();
		
		result = m_pDevice->vkGetRayTracingShaderGroupHandlesKHR(m_pDevice->Device, m_Pipeline, 0, (uint32)shaderGroups.size(), sbtSize, pMapped);
		if (result!= VK_SUCCESS)
		{
			if (pDesc->pName)
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkGetRayTracingShaderGroupHandlesKHR failed for \"%s\"", pDesc->pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkGetRayTracingShaderGroupHandlesKHR failed");
			}
			
			return false;
		}

		m_pShaderHandleStorageBuffer->Unmap();

		ICommandAllocator* pCommandAllocator = m_pDevice->CreateCommandAllocator("Ray Tracing Pipeline SBT Copy", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

		CommandListDesc commandListDesc = {};
		commandListDesc.pName			= "Ray Tracing Pipeline Command List";
		commandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		ICommandList* pCommandList = m_pDevice->CreateCommandList(pCommandAllocator, &commandListDesc);

		BufferDesc sbtDesc = {};
		sbtDesc.pName		= "Shader Binding Table";
		sbtDesc.Flags		= BUFFER_FLAG_COPY_DST | BUFFER_FLAG_RAY_TRACING;
		sbtDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		sbtDesc.SizeInBytes	= sbtSize;

		m_pShaderBindingTable = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&sbtDesc, nullptr));

		pCommandAllocator->Reset();

		pCommandList->Begin(nullptr);
		pCommandList->CopyBuffer(m_pShaderHandleStorageBuffer, 0, m_pShaderBindingTable, 0, sbtSize);
		pCommandList->End();

		pCommandQueue->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN , nullptr, 0, nullptr, 0);

		m_BindingStride						= shaderGroupHandleSize;
		
		m_BindingOffsetRaygenShaderGroup	= 0;
		m_BindingSizeRaygenShaderGroup		= m_BindingStride;
		
		m_BindingOffsetHitShaderGroup		= m_BindingOffsetRaygenShaderGroup + m_BindingSizeRaygenShaderGroup;
		m_BindingSizeHitShaderGroup			= m_BindingStride * pDesc->ClosestHitShaderCount;
		
		m_BindingOffsetMissShaderGroup		= m_BindingOffsetHitShaderGroup + m_BindingSizeHitShaderGroup;
		m_BindingSizeMissShaderGroup		= m_BindingStride * pDesc->MissShaderCount;

		SetName(pDesc->pName);
		if (pDesc->pName)
		{
			D_LOG_MESSAGE("[RayTracingPipelineStateVK]: Created Pipeline for %s", pDesc->pName);
		}
		else
		{
			D_LOG_MESSAGE("[RayTracingPipelineStateVK]: Created Pipeline");
		}

		SAFERELEASE(pCommandAllocator);
		SAFERELEASE(pCommandList);

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
	
	void RayTracingPipelineStateVK::CreateShaderStageInfo(const ShaderModuleDesc* pShaderModule, TArray<VkPipelineShaderStageCreateInfo>& shaderStagesInfos, 
		TArray<VkSpecializationInfo>& shaderStagesSpecializationInfos, TArray<TArray<VkSpecializationMapEntry>>& shaderStagesSpecializationMaps)
	{
		const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pShaderModule->pShader);

		// ShaderStageInfo
		VkPipelineShaderStageCreateInfo shaderCreateInfo = { };
		shaderCreateInfo.sType	= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderCreateInfo.pNext	= nullptr;
		shaderCreateInfo.flags	= 0;
		shaderCreateInfo.stage	= ConvertShaderStageFlag(pShader->GetDesc().Stage);
		shaderCreateInfo.module	= pShader->GetShaderModule();
		shaderCreateInfo.pName	= pShader->GetEntryPoint();

		// Shader Constants
		if (pShaderModule->ShaderConstantCount)
		{
			TArray<VkSpecializationMapEntry> specializationEntires(pShaderModule->ShaderConstantCount);
			for (uint32 i = 0; i < pShaderModule->ShaderConstantCount; i++)
			{
				VkSpecializationMapEntry specializationEntry = {};
				specializationEntry.constantID	= i;
				specializationEntry.offset		= i * sizeof(ShaderConstant);
				specializationEntry.size		= sizeof(ShaderConstant);
				specializationEntires.emplace_back(specializationEntry);
			}

			shaderStagesSpecializationMaps.emplace_back(specializationEntires);

			VkSpecializationInfo specializationInfo = { };
			specializationInfo.mapEntryCount	= static_cast<uint32>(specializationEntires.size());
			specializationInfo.pMapEntries		= specializationEntires.data();
			specializationInfo.dataSize			= pShaderModule->ShaderConstantCount * sizeof(ShaderConstant);
			specializationInfo.pData			= pShaderModule->pConstants;
			shaderStagesSpecializationInfos.emplace_back(specializationInfo);

			shaderCreateInfo.pSpecializationInfo = &shaderStagesSpecializationInfos.back();
		}
		else
		{
			shaderCreateInfo.pSpecializationInfo = nullptr;
		}

		shaderStagesInfos.emplace_back(shaderCreateInfo);
	}
}
