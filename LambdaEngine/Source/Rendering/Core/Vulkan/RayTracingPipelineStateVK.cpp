#include "Log/Log.h"

#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"
#include "Rendering/RenderSystem.h"

#include "Math/MathUtilities.h"

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

		SAFERELEASE(m_pSBT);
		SAFERELEASE(m_pShaderHandleStorageBuffer);
		SAFERELEASE(m_pCommandAllocator);
		SAFERELEASE(m_pCommandList);

		m_RaygenBufferRegion	= {};
		m_HitBufferRegion		= {};
		m_MissBufferRegion		= {};
		m_CallableBufferRegion	= {};
	}

	bool RayTracingPipelineStateVK::Init(const RayTracingPipelineStateDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);
		
		if (pDesc->pRaygenShader == nullptr)
		{
			LOG_ERROR("[RayTracingPipelineStateVK]: pRaygenShader cannot be nullptr!");
			return false;
		}

		if (pDesc->ClosestHitShaderCount == 0)
		{
			LOG_ERROR("[RayTracingPipelineStateVK]: ClosestHitShaderCount cannot be zero!");
			return false;
		}

		if (pDesc->MissShaderCount == 0)
		{
			LOG_ERROR("[RayTracingPipelineStateVK]: MissShaderCount cannot be zero!");
			return false;
		}

		// Define shader stage create infos
		std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos;
		std::vector<VkSpecializationInfo> shaderStagesSpecializationInfos;
		std::vector<std::vector<VkSpecializationMapEntry>> shaderStagesSpecializationMaps;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

		if (!CreateShaderData(shaderStagesInfos, shaderStagesSpecializationInfos, shaderStagesSpecializationMaps, shaderGroups, pDesc))
        {
			LOG_ERROR("[RayTracingPipelineStateVK]: Failed to create shader data!");
            return false;
        }

		VkPipelineLibraryCreateInfoKHR rayTracingPipelineLibrariesInfo = {};
		rayTracingPipelineLibrariesInfo.sType			= VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		rayTracingPipelineLibrariesInfo.libraryCount	= 0;
		rayTracingPipelineLibrariesInfo.pLibraries		= nullptr;

        const PipelineLayoutVK* pPipelineLayoutVk = reinterpret_cast<const PipelineLayoutVK*>(pDesc->pPipelineLayout);
        
		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo = {};
		rayTracingPipelineInfo.sType				= VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		//rayTracingPipelineInfo.flags				= VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR | VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR;
		rayTracingPipelineInfo.stageCount			= (uint32)shaderStagesInfos.size();
		rayTracingPipelineInfo.pStages				= shaderStagesInfos.data();
		rayTracingPipelineInfo.groupCount			= (uint32)shaderGroups.size();
		rayTracingPipelineInfo.pGroups				= shaderGroups.data();
		rayTracingPipelineInfo.maxRecursionDepth	= pDesc->MaxRecursionDepth;
		rayTracingPipelineInfo.layout				= pPipelineLayoutVk->GetPipelineLayout();
		rayTracingPipelineInfo.libraries			= rayTracingPipelineLibrariesInfo;

        VkResult result = m_pDevice->vkCreateRayTracingPipelinesKHR(m_pDevice->Device, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &m_Pipeline);
		if (result != VK_SUCCESS)
		{
            LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkCreateRayTracingPipelinesKHR failed for \"%s\"", pDesc->Name.c_str());
            
			return false;
		}

		uint64 shaderGroupBaseAlignment = m_pDevice->RayTracingProperties.shaderGroupBaseAlignment;
		uint64 shaderGroupHandleSize	= m_pDevice->RayTracingProperties.shaderGroupHandleSize;

		VkDeviceSize raygenUnalignedOffset	= 0;
		VkDeviceSize raygenAlignedOffset	= 0;
		VkDeviceSize raygenSize				= shaderGroupHandleSize;
		VkDeviceSize raygenStride			= shaderGroupHandleSize;

		VkDeviceSize hitUnalignedOffset		= raygenUnalignedOffset + raygenSize;
		VkDeviceSize hitAlignedOffset		= AlignUp(raygenAlignedOffset + raygenSize, shaderGroupBaseAlignment);;
		VkDeviceSize hitSize				= VkDeviceSize(pDesc->ClosestHitShaderCount) * VkDeviceSize(shaderGroupHandleSize);
		VkDeviceSize hitStride				= shaderGroupHandleSize;

		VkDeviceSize missUnalignedOffset	= hitUnalignedOffset + hitSize;
		VkDeviceSize missAlignedOffset		= AlignUp(hitAlignedOffset + hitSize, shaderGroupBaseAlignment);
		VkDeviceSize missSize				= VkDeviceSize(pDesc->MissShaderCount) * VkDeviceSize(shaderGroupHandleSize);
		VkDeviceSize missStride				= shaderGroupHandleSize;
		
		uint32 shaderHandleStorageSize		= missUnalignedOffset + missSize;
		uint32 sbtSize						= missAlignedOffset + missSize;

		BufferDesc shaderHandleStorageDesc = {};
		shaderHandleStorageDesc.pName			= "Shader Handle Storage";
		shaderHandleStorageDesc.Flags			= BUFFER_FLAG_COPY_SRC;
		shaderHandleStorageDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		shaderHandleStorageDesc.SizeInBytes		= shaderHandleStorageSize;

		m_pShaderHandleStorageBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&shaderHandleStorageDesc, nullptr));

		void* pMapped = m_pShaderHandleStorageBuffer->Map();
        
        result = m_pDevice->vkGetRayTracingShaderGroupHandlesKHR(m_pDevice->Device, m_Pipeline, 0, (uint32)shaderGroups.size(), shaderHandleStorageSize, pMapped);
		if (result!= VK_SUCCESS)
		{
            LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkGetRayTracingShaderGroupHandlesKHR failed for \"%s\"", pDesc->Name.c_str());
            
			return false;
		}

		m_pShaderHandleStorageBuffer->Unmap();

		m_pCommandAllocator					= m_pDevice->CreateCommandAllocator("Ray Tracing Pipeline SBT Copy", ECommandQueueType::COMMAND_QUEUE_COMPUTE);

		CommandListDesc commandListDesc = {};
		commandListDesc.pName				= "Ray Tracing Pipeline Command List";
		commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_PRIMARY;
		commandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_pCommandList						= m_pDevice->CreateCommandList(m_pCommandAllocator, &commandListDesc);

		BufferDesc sbtDesc = {};
		sbtDesc.pName			= "Shader Binding Table";
		sbtDesc.Flags			= BUFFER_FLAG_COPY_DST | BUFFER_FLAG_RAY_TRACING;
		sbtDesc.MemoryType		= EMemoryType::MEMORY_GPU;
		sbtDesc.SizeInBytes		= sbtSize;

		m_pSBT = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&sbtDesc, nullptr));

		m_pCommandAllocator->Reset();

		m_pCommandList->Begin(nullptr);
		m_pCommandList->CopyBuffer(m_pShaderHandleStorageBuffer, raygenUnalignedOffset, m_pSBT, raygenAlignedOffset,	raygenSize);
		m_pCommandList->CopyBuffer(m_pShaderHandleStorageBuffer, hitUnalignedOffset,	m_pSBT, hitAlignedOffset,		hitSize);
		m_pCommandList->CopyBuffer(m_pShaderHandleStorageBuffer, missUnalignedOffset,	m_pSBT, missAlignedOffset,		missSize);
		m_pCommandList->End();

		RenderSystem::GetComputeQueue()->ExecuteCommandLists(&m_pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN , nullptr, 0, nullptr, 0);

		VkBuffer sbtBuffer				= m_pSBT->GetBuffer();

		m_RaygenBufferRegion.buffer		= sbtBuffer;
		m_RaygenBufferRegion.offset		= raygenAlignedOffset;
		m_RaygenBufferRegion.size		= raygenSize;
		m_RaygenBufferRegion.stride		= raygenStride;

		m_HitBufferRegion.buffer		= sbtBuffer;
		m_HitBufferRegion.offset		= hitAlignedOffset;
		m_HitBufferRegion.size			= hitSize;
		m_HitBufferRegion.stride		= hitStride;

		m_MissBufferRegion.buffer		= sbtBuffer;
		m_MissBufferRegion.offset		= missAlignedOffset;
		m_MissBufferRegion.size			= missSize;
		m_MissBufferRegion.stride		= missStride;

		SetName(pDesc->Name.c_str());

		D_LOG_MESSAGE("[RayTracingPipelineStateVK]: Created Pipeline for %s", pDesc->Name.c_str());

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
		const RayTracingPipelineStateDesc* pDesc)
	{
		//Raygen Shader
		{
			const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->pRaygenShader);

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

			pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
			pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

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
		for (uint32 i = 0; i < pDesc->ClosestHitShaderCount; i++)
		{
			const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->ppClosestHitShaders[i]);

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

			pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
			pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

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
		for (uint32 i = 0; i < pDesc->MissShaderCount; i++)
		{
			const ShaderVK* pShader = reinterpret_cast<const ShaderVK*>(pDesc->ppMissShaders[i]);

			VkPipelineShaderStageCreateInfo shaderCreateInfo;
			VkSpecializationInfo shaderSpecializationInfo;
			std::vector<VkSpecializationMapEntry> shaderSpecializationMapEntries;

			pShader->FillSpecializationInfo(shaderSpecializationInfo, shaderSpecializationMapEntries);
			pShader->FillShaderStageInfo(shaderCreateInfo, &shaderSpecializationInfo);

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
