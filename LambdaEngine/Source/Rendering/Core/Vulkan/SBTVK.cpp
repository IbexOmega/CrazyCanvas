#include "Log/Log.h"

#include "Rendering/Core/Vulkan/RayTracingPipelineStateVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/SBTVK.h"

#include "Rendering/RenderAPI.h"

namespace LambdaEngine
{
	SBTVK::SBTVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	SBTVK::~SBTVK()
	{
		m_RaygenBufferRegion	= {};
		m_HitBufferRegion		= {};
		m_MissBufferRegion		= {};
		m_CallableBufferRegion	= {};
	}

	bool SBTVK::Init(CommandQueue* pCommandQueue, const SBTDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);
		VALIDATE(pDesc->pPipelineState != nullptr);
		VALIDATE(pDesc->pPipelineState->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING);

		void* pMapped;
		m_NumShaderRecords = pDesc->SBTRecords.GetSize();

		BufferDesc shaderRecordsBufferDesc = {};
		shaderRecordsBufferDesc.DebugName		= "Shader Records Storage";
		shaderRecordsBufferDesc.Flags			= BUFFER_FLAG_COPY_SRC;
		shaderRecordsBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		shaderRecordsBufferDesc.SizeInBytes		= m_NumShaderRecords * sizeof(SBTRecord);

		m_ShaderRecordsBuffer		= reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&shaderRecordsBufferDesc));

		pMapped = m_ShaderRecordsBuffer->Map();
		//memcpy(pMapped, pDesc->SBTRecords.GetData(), shaderRecordsBufferDesc.SizeInBytes);
		memcpy(pMapped, pDesc->SBTRecords.GetData(), shaderRecordsBufferDesc.SizeInBytes);
		m_ShaderRecordsBuffer->Unmap();

		const RayTracingPipelineStateVK* pRayTracingPipelineVK = reinterpret_cast<const RayTracingPipelineStateVK*>(pDesc->pPipelineState);

		uint64 shaderGroupBaseAlignment = m_pDevice->RayTracingProperties.shaderGroupBaseAlignment;
		uint64 shaderGroupHandleSize	= m_pDevice->RayTracingProperties.shaderGroupHandleSize;

		VkDeviceSize raygenUnalignedOffset	= 0;
		VkDeviceSize raygenAlignedOffset	= 0;
		VkDeviceSize raygenSize				= shaderGroupHandleSize;
		VkDeviceSize raygenStride			= shaderGroupHandleSize;

		VkDeviceSize hitGroupHandleOffset	= raygenUnalignedOffset + raygenSize;
		VkDeviceSize hitSBTOffset			= AlignUp(raygenAlignedOffset + raygenSize, shaderGroupBaseAlignment);
		VkDeviceSize hitGroupHandleStride	= shaderGroupHandleSize;
		VkDeviceSize hitSBTStride			= AlignUp(shaderGroupHandleSize + sizeof(SBTRecord), shaderGroupBaseAlignment);
		VkDeviceSize hitGroupHandleSize		= pRayTracingPipelineVK->HitShaderCount() * hitGroupHandleStride;
		VkDeviceSize hitSBTSize				= pDesc->SBTRecords.GetSize() * pRayTracingPipelineVK->HitShaderCount() * hitSBTStride;

		VkDeviceSize missGroupHandleOffset	= hitGroupHandleOffset + hitGroupHandleSize;
		VkDeviceSize missSBTOffset			= AlignUp(hitSBTOffset + hitSBTSize, shaderGroupBaseAlignment);
		VkDeviceSize missSize				= pRayTracingPipelineVK->MissShaderCount() * VkDeviceSize(shaderGroupHandleSize);
		VkDeviceSize missStride				= shaderGroupHandleSize;
		
		uint64 shaderHandleStorageSize		= missGroupHandleOffset + missSize;
		uint64 sbtSize						= missSBTOffset + missSize;
		uint32 shaderGroupCount				= 1 + pRayTracingPipelineVK->HitShaderCount() + pRayTracingPipelineVK->MissShaderCount();

		BufferDesc shaderHandleStorageDesc = {};
		shaderHandleStorageDesc.DebugName		= "Shader Handle Storage";
		shaderHandleStorageDesc.Flags			= BUFFER_FLAG_COPY_SRC;
		shaderHandleStorageDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		shaderHandleStorageDesc.SizeInBytes		= shaderHandleStorageSize;

		m_ShaderHandleStorageBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&shaderHandleStorageDesc));

		pMapped = m_ShaderHandleStorageBuffer->Map();
		VkResult result = m_pDevice->vkGetRayTracingShaderGroupHandlesKHR(m_pDevice->Device, pRayTracingPipelineVK->GetPipeline(), 0, shaderGroupCount, shaderHandleStorageSize, pMapped);
		if (result!= VK_SUCCESS)
		{
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkGetRayTracingShaderGroupHandlesKHR failed for \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[RayTracingPipelineStateVK]: vkGetRayTracingShaderGroupHandlesKHR failed");
			}
			
			return false;
		}
		m_ShaderHandleStorageBuffer->Unmap();

		CommandAllocator* pCommandAllocator = m_pDevice->CreateCommandAllocator("Ray Tracing Pipeline SBT Copy", ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);
		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName		= "Ray Tracing Pipeline Command List";
		commandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags			= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		CommandList* pCommandList = m_pDevice->CreateCommandList(pCommandAllocator, &commandListDesc);

		BufferDesc sbtDesc = {};
		sbtDesc.DebugName	= "Shader Binding Table";
		sbtDesc.Flags		= BUFFER_FLAG_COPY_DST | BUFFER_FLAG_RAY_TRACING;
		sbtDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		sbtDesc.SizeInBytes	= sbtSize;

		m_SBTBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&sbtDesc));
		pCommandAllocator->Reset();

		pCommandList->Begin(nullptr);
		pCommandList->CopyBuffer(m_ShaderHandleStorageBuffer.Get(), raygenUnalignedOffset,	m_SBTBuffer.Get(), raygenAlignedOffset,	raygenSize);

		for (VkDeviceSize s = 0; s < m_NumShaderRecords; s++)
		{
			VkDeviceSize baseOffset = hitSBTOffset + s * hitSBTStride;
			pCommandList->CopyBuffer(m_ShaderHandleStorageBuffer.Get(), hitGroupHandleOffset,	m_SBTBuffer.Get(), baseOffset,							shaderGroupHandleSize);
			pCommandList->CopyBuffer(m_ShaderRecordsBuffer.Get(),		s * sizeof(SBTRecord),	m_SBTBuffer.Get(), baseOffset + shaderGroupHandleSize,	sizeof(SBTRecord));
		}

		pCommandList->CopyBuffer(m_ShaderHandleStorageBuffer.Get(), missGroupHandleOffset,	m_SBTBuffer.Get(), missSBTOffset, missSize);
		pCommandList->End();

		pCommandQueue->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN , nullptr, 0, nullptr, 0);

		VkBuffer sbtBuffer = m_SBTBuffer->GetBuffer();
		m_RaygenBufferRegion.buffer	= sbtBuffer;
		m_RaygenBufferRegion.offset	= raygenAlignedOffset;
		m_RaygenBufferRegion.size	= raygenSize;
		m_RaygenBufferRegion.stride	= raygenStride;

		m_HitBufferRegion.buffer	= sbtBuffer;
		m_HitBufferRegion.offset	= hitSBTOffset;
		m_HitBufferRegion.size		= hitSBTSize;
		m_HitBufferRegion.stride	= hitSBTStride;

		m_MissBufferRegion.buffer	= sbtBuffer;
		m_MissBufferRegion.offset	= missSBTOffset;
		m_MissBufferRegion.size		= missSize;
		m_MissBufferRegion.stride	= missStride;

		pCommandQueue->Flush();
		SAFERELEASE(pCommandAllocator);
		SAFERELEASE(pCommandList);

		if (!pDesc->DebugName.empty())
		{
			D_LOG_MESSAGE("[SBTVK]: Created SBT %s", pDesc->DebugName.c_str());
		}
		else
		{
			D_LOG_MESSAGE("[SBTVK]: Created SBT");
		}

		return true;
	}

	void SBTVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName((debugName + " Handle Storage"), reinterpret_cast<uint64>(m_ShaderHandleStorageBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
		m_pDevice->SetVulkanObjectName((debugName + " SBT Buffer"), reinterpret_cast<uint64>(m_SBTBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
		m_pDevice->SetVulkanObjectName((debugName + " Records Buffer"), reinterpret_cast<uint64>(m_ShaderRecordsBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
		m_DebugName = debugName;
	}
}