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
		SAFERELEASE(m_pShaderHandleStorageBuffer);
		SAFERELEASE(m_pSBTBuffer);
		SAFERELEASE(m_pShaderRecordsBuffer);

		m_RaygenBufferRegion	= {};
		m_HitBufferRegion		= {};
		m_MissBufferRegion		= {};
		m_CallableBufferRegion	= {};
	}

	bool SBTVK::Init(CommandList* pCommandList, const SBTDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		TArray<DeviceChild*> removedDeviceChildren;
		if (!Build(pCommandList, removedDeviceChildren, pDesc))
		{
			LOG_ERROR("[SBTVK]: Failed to build SBT");
			return false;
		}

		for (DeviceChild* pDeviceChild : removedDeviceChildren)
		{
			SAFERELEASE(pDeviceChild);
		}

		if (!pDesc->DebugName.empty())
		{
			LOG_DEBUG("[SBTVK]: Created SBT %s", pDesc->DebugName.c_str());
		}
		else
		{
			LOG_DEBUG("[SBTVK]: Created SBT");
		}

		return true;
	}

	bool SBTVK::Build(CommandList* pCommandList, TArray<DeviceChild*>& removedDeviceResources, const SBTDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);
		VALIDATE(pDesc->pPipelineState != nullptr);
		VALIDATE(pDesc->pPipelineState->GetType() == EPipelineStateType::PIPELINE_STATE_TYPE_RAY_TRACING);

		constexpr const uint64 OVER_ALLOCATION_MULT = 2;

		void* pMapped;
		m_NumShaderRecords = pDesc->SBTRecords.GetSize();

		uint64 newShaderRecordsBufferSize = m_NumShaderRecords * sizeof(SBTRecord);

		if (m_pShaderRecordsBuffer == nullptr || newShaderRecordsBufferSize > m_pShaderRecordsBuffer->GetDesc().SizeInBytes)
		{
			if (m_pShaderRecordsBuffer != nullptr) removedDeviceResources.PushBack(m_pShaderRecordsBuffer);

			BufferDesc shaderRecordsBufferDesc = {};
			shaderRecordsBufferDesc.DebugName		= "Shader Records Storage";
			shaderRecordsBufferDesc.Flags			= BUFFER_FLAG_COPY_SRC;
			shaderRecordsBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			shaderRecordsBufferDesc.SizeInBytes		= OVER_ALLOCATION_MULT * newShaderRecordsBufferSize;

			m_pShaderRecordsBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&shaderRecordsBufferDesc));
		}

		pMapped = m_pShaderRecordsBuffer->Map();
		memcpy(pMapped, pDesc->SBTRecords.GetData(), newShaderRecordsBufferSize);
		m_pShaderRecordsBuffer->Unmap();

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

		if (m_pShaderHandleStorageBuffer == nullptr || shaderHandleStorageSize > m_pShaderHandleStorageBuffer->GetDesc().SizeInBytes)
		{
			if (m_pShaderHandleStorageBuffer != nullptr) removedDeviceResources.PushBack(m_pShaderHandleStorageBuffer);

			BufferDesc shaderHandleStorageDesc = {};
			shaderHandleStorageDesc.DebugName		= "Shader Handle Storage";
			shaderHandleStorageDesc.Flags			= BUFFER_FLAG_COPY_SRC;
			shaderHandleStorageDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			shaderHandleStorageDesc.SizeInBytes		= OVER_ALLOCATION_MULT * shaderHandleStorageSize;

			m_pShaderHandleStorageBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&shaderHandleStorageDesc));
		}

		pMapped = m_pShaderHandleStorageBuffer->Map();
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
		m_pShaderHandleStorageBuffer->Unmap();

		if (m_pSBTBuffer == nullptr || sbtSize > m_pSBTBuffer->GetDesc().SizeInBytes)
		{
			if (m_pSBTBuffer != nullptr) removedDeviceResources.PushBack(m_pSBTBuffer);

			BufferDesc sbtDesc = {};
			sbtDesc.DebugName	= "Shader Binding Table";
			sbtDesc.Flags		= BUFFER_FLAG_COPY_DST | BUFFER_FLAG_RAY_TRACING;
			sbtDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			sbtDesc.SizeInBytes	= OVER_ALLOCATION_MULT * sbtSize;

			m_pSBTBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&sbtDesc));
		}

		pCommandList->CopyBuffer(m_pShaderHandleStorageBuffer, raygenUnalignedOffset, m_pSBTBuffer, raygenAlignedOffset,	raygenSize);

		for (VkDeviceSize s = 0; s < m_NumShaderRecords; s++)
		{
			VkDeviceSize baseOffset = hitSBTOffset + s * hitSBTStride;
			VkDeviceSize hitGroupByteOffset = pDesc->HitGroupIndices[(uint32_t)s] * shaderGroupHandleSize;
			pCommandList->CopyBuffer(m_pShaderHandleStorageBuffer, hitGroupHandleOffset + hitGroupByteOffset,	m_pSBTBuffer, baseOffset,							shaderGroupHandleSize);
			pCommandList->CopyBuffer(m_pShaderRecordsBuffer,		s * sizeof(SBTRecord),	m_pSBTBuffer, baseOffset + shaderGroupHandleSize,	sizeof(SBTRecord));
		}

		pCommandList->CopyBuffer(m_pShaderHandleStorageBuffer, missGroupHandleOffset, m_pSBTBuffer, missSBTOffset, missSize);

		VkBuffer sbtBuffer = m_pSBTBuffer->GetBuffer();
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

		return true;
	}

	void SBTVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName((debugName + " Handle Storage"), reinterpret_cast<uint64>(m_pShaderHandleStorageBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
		m_pDevice->SetVulkanObjectName((debugName + " SBT Buffer"), reinterpret_cast<uint64>(m_pSBTBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
		m_pDevice->SetVulkanObjectName((debugName + " Records Buffer"), reinterpret_cast<uint64>(m_pShaderRecordsBuffer->GetBuffer()), VK_OBJECT_TYPE_BUFFER);
		m_DebugName = debugName;
	}
}