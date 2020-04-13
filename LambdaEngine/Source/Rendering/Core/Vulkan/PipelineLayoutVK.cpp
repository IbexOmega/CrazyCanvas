#include "Log/Log.h"

#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	PipelineLayoutVK::PipelineLayoutVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_DescriptorCount()
	{
		memset(m_DescriptorSetLayouts, 0, sizeof(m_DescriptorSetLayouts));
	}

	PipelineLayoutVK::~PipelineLayoutVK()
	{
		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_pDevice->Device, m_PipelineLayout, nullptr);
			m_PipelineLayout = VK_NULL_HANDLE;
		}

		for (uint32 i = 0; i < m_DescriptorSetCount; i++)
		{
			if (m_DescriptorSetLayouts[i] != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(m_pDevice->Device, m_DescriptorSetLayouts[i], nullptr);
				m_DescriptorSetLayouts[i] = VK_NULL_HANDLE;
			}
		}

		for (uint32 i = 0; i < m_ImmutableSamplerCount; i++)
		{
			SAFERELEASE(m_ppImmutableSamplers[i]);
		}
	}

	bool PipelineLayoutVK::Init(const PipelineLayoutDesc& desc)
	{
		VkPushConstantRange		constantRanges[MAX_CONSTANT_RANGES];
		DescriptorSetLayoutData	descriptorSetLayouts[MAX_DESCRIPTOR_SET_LAYOUTS];
		
		CreatePushConstantRanges(desc.pConstantRanges, desc.ConstantRangeCount, constantRanges);
		CreateDescriptorSetLayout(desc.pDescriptorSetLayouts, desc.DescriptorSetLayoutCount, descriptorSetLayouts);

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
		pipelineLayoutCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext					= nullptr;
		pipelineLayoutCreateInfo.flags					= 0;
		pipelineLayoutCreateInfo.pSetLayouts			= m_DescriptorSetLayouts;
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

	void PipelineLayoutVK::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc* pDescriptorSetLayouts, uint32 descriptorSetLayoutCount, DescriptorSetLayoutData* pResultDescriptorSetLayouts)
	{
		m_DescriptorSetCount	= 0;
		m_ImmutableSamplerCount = 0;

		//DescriptorSetLayouts
		for (uint32 descriptorSetIndex = 0; descriptorSetIndex < descriptorSetLayoutCount; descriptorSetIndex++)
		{
			DescriptorSetLayoutData&		descriptorSet		= pResultDescriptorSetLayouts[descriptorSetIndex];
			const DescriptorSetLayoutDesc&	descriptorSetLayout	= pDescriptorSetLayouts[descriptorSetIndex];

			//Bindings for each descriptorsetlayout
			descriptorSet.DescriptorCount = descriptorSetLayout.DescriptorBindingCount;
			for (uint32 bindingIndex = 0; bindingIndex < descriptorSet.DescriptorCount; bindingIndex++)
			{
				VkDescriptorSetLayoutBinding&	bindingVk	= descriptorSet.DescriptorBindings[bindingIndex];
				const DescriptorBindingDesc&	binding		= descriptorSetLayout.pDescriptorBindings[bindingIndex];

				// Immutable Samplers for each binding
				ImmutableSamplersData& immutableSampler = descriptorSet.ImmutableSamplers[bindingIndex];
				if (binding.ppImmutableSamplers)
				{
					for (uint32 samplerIndex = 0; samplerIndex < binding.DescriptorCount; descriptorSetIndex++)
					{
						SamplerVK* pSamplerVk = reinterpret_cast<SamplerVK*>(binding.ppImmutableSamplers[samplerIndex]);
						pSamplerVk->AddRef();

						//Store samplers to make sure that they are not released before we destroy the pipelinelayout
						m_ppImmutableSamplers[m_ImmutableSamplerCount] = pSamplerVk;
						m_ImmutableSamplerCount++;

						immutableSampler.ImmutableSamplers[samplerIndex] = pSamplerVk->GetSampler();
					}
				}

				bindingVk.descriptorType		= ConvertDescriptorType(binding.DescriptorType);
				bindingVk.binding				= binding.Binding;
				bindingVk.descriptorCount		= binding.DescriptorCount;
				bindingVk.pImmutableSamplers	= binding.ppImmutableSamplers != nullptr ? immutableSampler.ImmutableSamplers : nullptr;
				bindingVk.stageFlags			= ConvertShaderStageMask(binding.ShaderStageMask);
			}

			descriptorSet.CreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSet.CreateInfo.pNext			= nullptr;
			descriptorSet.CreateInfo.flags			= 0;
			descriptorSet.CreateInfo.bindingCount	= descriptorSet.DescriptorCount;
			descriptorSet.CreateInfo.pBindings		= descriptorSet.DescriptorBindings;

			VkResult result = vkCreateDescriptorSetLayout(m_pDevice->Device, &descriptorSet.CreateInfo, nullptr, &m_DescriptorSetLayouts[m_DescriptorSetCount]);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[PipelineLayoutVK]: Failed to create DescriptorSetLayout[%d]", m_DescriptorSetCount);
				return;
			}
			else
			{
				D_LOG_MESSAGE("[PipelineLayoutVK]: Created DescriptorSetLayout[%d]", m_DescriptorSetCount);
				m_DescriptorSetCount++;
			}
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