#include "Log/Log.h"

#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	PipelineLayoutVK::PipelineLayoutVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_DescriptorCount()
	{
	}

	PipelineLayoutVK::~PipelineLayoutVK()
	{
		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_pDevice->Device, m_PipelineLayout, nullptr);
			m_PipelineLayout = VK_NULL_HANDLE;
		}

		if (m_DescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_pDevice->Device, m_DescriptorSetLayout, nullptr);
			m_DescriptorSetLayout = VK_NULL_HANDLE;
		}
	}

	bool PipelineLayoutVK::Init(const PipelineLayoutDesc& desc)
	{
		VkPushConstantRange constantRanges[MAX_CONSTANT_RANGES];


		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
		pipelineLayoutCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext					= nullptr;
		pipelineLayoutCreateInfo.flags					= 0;
		//pipelineLayoutCreateInfo.pSetLayouts			= ;
		pipelineLayoutCreateInfo.setLayoutCount			= desc.DescriptorSetLayoutCount;
		pipelineLayoutCreateInfo.pPushConstantRanges	= constantRanges;
		pipelineLayoutCreateInfo.pushConstantRangeCount = desc.ConstantRangeCount;

		VkResult result = vkCreatePipelineLayout(m_pDevice->Device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);
		if (result != VK_SUCCESS)
		{
			if (desc.pName)
			{
				LOG_VULKAN_ERROR(result, "[PipelineLayoutVK]: Failed to create PipelineLayout \"%s\"", desc.pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[PipelineLayoutVK]: Failed to create PipelineLayout");
			}

			return false;
		}
		else
		{
			SetName(desc.pName);

			if (desc.pName)
			{
				D_LOG_MESSAGE("[PipelineLayoutVK]: Created PipelineLayout \"%s\"", desc.pName);
			}
			else
			{
				D_LOG_MESSAGE("[PipelineLayoutVK]: Created PipelineLayout");
			}
		}

		return true;
	}

	void PipelineLayoutVK::CreatePushConstantRanges(const ConstantRangeDesc* pConstantRanges, uint32 constantRangeCount, VkPushConstantRange* pResultConstantRanges)
	{
		for (uint32 i = 0; i < constantRangeCount; i++)
		{
			VkPushConstantRange&	constantRangeVk	= pResultConstantRanges[i];
			const ConstantRangeDesc constantRange	= pConstantRanges[i];

			constantRangeVk.offset		= constantRange.OffsetInBytes;
			constantRangeVk.size		= constantRange.SizeInBytes;
			constantRangeVk.stageFlags	= ConvertShaderStageMask(constantRange.ShaderStageFlags);
		}
	}

	void PipelineLayoutVK::CreateDescriptorSetLayouta(const DescriptorSetLayoutDesc* pDescriptorSetLayouts, uint32 descriptorSetLayoutCount, VkDescriptorSetLayoutCreateInfo* pResultDescriptorSetLayouts)
	{
		for (uint32 i = 0; i < descriptorSetLayoutCount; i++)
		{
			VkDescriptorSetLayoutCreateInfo&	descriptorSetLayoutVk	= pResultDescriptorSetLayouts[i];
			const DescriptorSetLayoutDesc&		descriptorSetLayout		= pDescriptorSetLayouts[i];

			descriptorSetLayoutVk.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutVk.pNext			= nullptr;
			descriptorSetLayoutVk.flags			= 0;
			descriptorSetLayoutVk.pBindings		= 0;
			//descriptorSetLayoutVk.bindingCount	= ;
		}
	}

	void PipelineLayoutVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_PipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT);

			//TODO: Set name of all descriptorset layouts
		}
	}
}