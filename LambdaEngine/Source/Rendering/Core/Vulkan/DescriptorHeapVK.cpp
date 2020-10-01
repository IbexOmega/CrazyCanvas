#include "Log/Log.h"

#include "Rendering/Core/Vulkan/DescriptorHeapVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	DescriptorHeapVK::DescriptorHeapVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}
	
	DescriptorHeapVK::~DescriptorHeapVK()
	{
		if (m_DescriptorHeap != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_pDevice->Device, m_DescriptorHeap, nullptr);
			m_DescriptorHeap = VK_NULL_HANDLE;
		}
	}
	
	bool DescriptorHeapVK::Init(const DescriptorHeapDesc* pDesc)
	{
		constexpr uint32 DESCRIPTOR_TYPE_COUNT = 7;
		VkDescriptorPoolSize poolSizes[DESCRIPTOR_TYPE_COUNT];
		uint32 poolCount = 0;

		if (pDesc->DescriptorCount.UnorderedAccessBufferDescriptorCount > 0)
		{
			poolSizes[poolCount].type				= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			poolSizes[poolCount].descriptorCount	= pDesc->DescriptorCount.UnorderedAccessBufferDescriptorCount;
			poolCount++;
		}

		if (pDesc->DescriptorCount.UnorderedAccessTextureDescriptorCount > 0)
		{
			poolSizes[poolCount].type				= VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			poolSizes[poolCount].descriptorCount	= pDesc->DescriptorCount.UnorderedAccessTextureDescriptorCount;
			poolCount++;
		}

		if (pDesc->DescriptorCount.ConstantBufferDescriptorCount > 0)
		{
			poolSizes[poolCount].type				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[poolCount].descriptorCount	= pDesc->DescriptorCount.ConstantBufferDescriptorCount;
		}

		if (pDesc->DescriptorCount.TextureCombinedSamplerDescriptorCount > 0)
		{
			poolSizes[poolCount].type				= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[poolCount].descriptorCount	= pDesc->DescriptorCount.TextureCombinedSamplerDescriptorCount;
			poolCount++;
		}

		if (pDesc->DescriptorCount.TextureDescriptorCount > 0)
		{
			poolSizes[poolCount].type				= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			poolSizes[poolCount].descriptorCount	= pDesc->DescriptorCount.TextureDescriptorCount;
			poolCount++;
		}

		if (pDesc->DescriptorCount.AccelerationStructureDescriptorCount > 0)
		{
			poolSizes[poolCount].type				= VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			poolSizes[poolCount].descriptorCount	= pDesc->DescriptorCount.AccelerationStructureDescriptorCount;
			poolCount++;
		}

		if (pDesc->DescriptorCount.SamplerDescriptorCount > 0)
		{
			poolSizes[poolCount].type				= VK_DESCRIPTOR_TYPE_SAMPLER;
			poolSizes[poolCount].descriptorCount	= pDesc->DescriptorCount.SamplerDescriptorCount;
			poolCount++;
		}

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		if (pDesc->Flags & FDescriptorHeapFlag::DESCRIPTOR_HEAP_FLAG_UPDATE_AFTER_BIND_POOL)
		{
			poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		}

		poolInfo.poolSizeCount	= poolCount;
		poolInfo.pPoolSizes		= poolSizes;
		poolInfo.maxSets		= pDesc->DescriptorSetCount;
		
		VkResult result = vkCreateDescriptorPool(m_pDevice->Device, &poolInfo, nullptr, &m_DescriptorHeap);
		if (result != VK_SUCCESS)
		{
			if (pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "[DescriptorHeapVK]: Failed to create DescriptorHeap \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[DescriptorHeapVK]: Failed to create DescriptorHeap");
			}

			return false;
		}
		else
		{
			m_Desc			= *pDesc;
			m_HeapStatus	= pDesc->DescriptorCount;
			SetName(pDesc->DebugName);
		
			if (pDesc->DebugName.empty())
			{
				D_LOG_MESSAGE("[DescriptorHeapVK]: Created DescriptorHeap \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				D_LOG_MESSAGE("[DescriptorHeapVK]: Created DescriptorHeap");
			}

			return true;
		}
	}

	VkDescriptorSet DescriptorHeapVK::AllocateDescriptorSet(const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex)
	{
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		const PipelineLayoutVK* pPipelineLayoutVk	= reinterpret_cast<const PipelineLayoutVK*>(pPipelineLayout);
		VkDescriptorSetLayout	descriptorSetLayout = pPipelineLayoutVk->GetDescriptorSetLayout(descriptorLayoutIndex);

		DescriptorHeapInfo info			= pPipelineLayoutVk->GetDescriptorHeapInfo(descriptorLayoutIndex);
		DescriptorHeapInfo newStatus	= m_HeapStatus - info;

#ifndef LAMBDA_PRODUCTION
		if (!newStatus.IsValid())
		{
			LOG_ERROR("[DescriptorHeapVK]: Not enough descriptors in DescriptorHeap for allocation");
			return VK_NULL_HANDLE;
		}
#endif

		VkDescriptorSetAllocateInfo allocate = {};
		allocate.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocate.pNext				= nullptr;
		allocate.pSetLayouts		= &descriptorSetLayout;
		allocate.descriptorSetCount = 1;
		allocate.descriptorPool		= m_DescriptorHeap;

		VkResult result = vkAllocateDescriptorSets(m_pDevice->Device, &allocate, &descriptorSet);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[DescriptorHeapVK]: Failed to allocate descriptorset");
			return VK_NULL_HANDLE;
		}
		else
		{
			m_HeapStatus = newStatus;
			return descriptorSet;
		}
	}

	void DescriptorHeapVK::FreeDescriptorSet(VkDescriptorSet descriptorSet)
	{
		VkResult result = vkFreeDescriptorSets(m_pDevice->Device, m_DescriptorHeap, 1, &descriptorSet);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[DescriptorHeapVK]: Failed to allocate descriptorset");
		}
	}

	void DescriptorHeapVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_DescriptorHeap), VK_OBJECT_TYPE_DESCRIPTOR_POOL);
		m_Desc.DebugName = debugName;
	}
}
