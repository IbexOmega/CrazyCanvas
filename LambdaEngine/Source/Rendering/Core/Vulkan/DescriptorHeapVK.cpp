#include "Log/Log.h"

#include "Rendering/Core/Vulkan/DescriptorHeapVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	DescriptorHeapVK::DescriptorHeapVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_HeapStatus(),
		m_Desc()
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
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[0].descriptorCount = pDesc->DescriptorCount.UnorderedAccessBufferDescriptorCount;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[1].descriptorCount = pDesc->DescriptorCount.UnorderedAccessTextureDescriptorCount;

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[2].descriptorCount = pDesc->DescriptorCount.ConstantBufferDescriptorCount;

		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = pDesc->DescriptorCount.TextureCombinedSamplerDescriptorCount;

		poolSizes[4].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[4].descriptorCount = pDesc->DescriptorCount.TextureDescriptorCount;

		poolSizes[5].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		poolSizes[5].descriptorCount = pDesc->DescriptorCount.AccelerationStructureDescriptorCount;

		poolSizes[6].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[6].descriptorCount = pDesc->DescriptorCount.SamplerDescriptorCount;

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext			= nullptr;
		poolInfo.flags			= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.poolSizeCount	= DESCRIPTOR_TYPE_COUNT;
		poolInfo.pPoolSizes		= poolSizes;
		poolInfo.maxSets		= pDesc->DescriptorCount.DescriptorSetCount;
		
		VkResult result = vkCreateDescriptorPool(m_pDevice->Device, &poolInfo, nullptr, &m_DescriptorHeap);
		if (result != VK_SUCCESS)
		{
			if (pDesc->pName)
			{
				LOG_VULKAN_ERROR(result, "[DescriptorHeapVK]: Failed to create DescriptorHeap \"%s\"", pDesc->pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[DescriptorHeapVK]: Failed to create DescriptorHeap");
			}

			return false;
		}
		else
		{
            memcpy(&m_Desc, pDesc, sizeof(m_Desc));
			m_HeapStatus	= pDesc->DescriptorCount;
			SetName(pDesc->pName);
		
			if (pDesc->pName)
			{
				D_LOG_MESSAGE("[DescriptorHeapVK]: Created DescriptorHeap \"%s\"", pDesc->pName);
			}
			else
			{
				D_LOG_MESSAGE("[DescriptorHeapVK]: Created DescriptorHeap");
			}

			return true;
		}
	}

	VkDescriptorSet DescriptorHeapVK::AllocateDescriptorSet(const IPipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex)
	{
		const PipelineLayoutVK* pPipelineLayoutVk	= reinterpret_cast<const PipelineLayoutVK*>(pPipelineLayout);
		VkDescriptorSetLayout	descriptorSetLayout = pPipelineLayoutVk->GetDescriptorSetLayout(descriptorLayoutIndex);

		DescriptorCountDesc count		= pPipelineLayoutVk->GetDescriptorCount(descriptorLayoutIndex);
		DescriptorCountDesc newStatus	= m_HeapStatus;
		newStatus.DescriptorSetCount--;
		newStatus.ConstantBufferDescriptorCount			-= count.ConstantBufferDescriptorCount;
		newStatus.AccelerationStructureDescriptorCount	-= count.AccelerationStructureDescriptorCount;
		newStatus.SamplerDescriptorCount				-= count.SamplerDescriptorCount;
		newStatus.TextureCombinedSamplerDescriptorCount -= count.TextureCombinedSamplerDescriptorCount;
		newStatus.TextureDescriptorCount				-= count.TextureDescriptorCount;
		newStatus.UnorderedAccessBufferDescriptorCount	-= count.UnorderedAccessBufferDescriptorCount;
		newStatus.UnorderedAccessTextureDescriptorCount	-= count.UnorderedAccessTextureDescriptorCount;

#ifndef LAMBDA_PRODUCTION
		if (!CheckValidDescriptorCount(newStatus))
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

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
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

	void DescriptorHeapVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_DescriptorHeap, VK_OBJECT_TYPE_DESCRIPTOR_POOL);
		}
	}
}
