#include "Log/Log.h"

#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	PipelineLayoutVK::PipelineLayoutVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice),
		m_DescriptorCounts()
	{
	}

	PipelineLayoutVK::~PipelineLayoutVK()
	{
		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_pDevice->Device, m_PipelineLayout, nullptr);
			m_PipelineLayout = VK_NULL_HANDLE;
		}

		for (VkDescriptorSetLayout& descriptorSetLayout : m_DescriptorSetLayouts)
		{
			if (descriptorSetLayout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(m_pDevice->Device, descriptorSetLayout, nullptr);
				descriptorSetLayout = VK_NULL_HANDLE;
			}
		}
	}

	bool PipelineLayoutVK::Init(const PipelineLayoutDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		// Push Constant Ranges
		TArray<VkPushConstantRange> pushConstants;
		for (const ConstantRangeDesc& constantRange : pDesc->ConstantRanges)
		{
			VkPushConstantRange	constantRangeVk = { };
			constantRangeVk.offset		= constantRange.OffsetInBytes;
			constantRangeVk.size		= constantRange.SizeInBytes;
			constantRangeVk.stageFlags	= ConvertShaderStageMask(constantRange.ShaderStageFlags);
			pushConstants.emplace_back(constantRangeVk);
		}

		// DescriptorSetLayouts
		TArray<VkSampler>						immutableSamplers;
		TArray<VkDescriptorSetLayoutBinding>	layoutBindings;
		for (const DescriptorSetLayoutDesc& descriptorSetLayout : pDesc->DescriptorSetLayouts)
		{
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;

			// Bindings for each descriptorsetlayout
			for (const DescriptorBindingDesc& binding : descriptorSetLayout.DescriptorBindings)
			{
				DescriptorHeapInfo				heapInfo;
				VkDescriptorSetLayoutBinding	bindingVk = { };

				// Immutable Samplers for each binding
				if (!binding.ImmutableSamplers.empty())
				{
					for (uint32 samplerIndex = 0; samplerIndex < binding.DescriptorCount; samplerIndex++)
					{
						// Store samplers to make sure that they are NOT released before we destroy the pipelinelayout
						const SamplerVK* pSamplerVk = reinterpret_cast<const SamplerVK*>(binding.ImmutableSamplers[samplerIndex].Get());
						m_ImmutableSamplers.emplace_back(binding.ImmutableSamplers[samplerIndex]);
						immutableSamplers.emplace_back(pSamplerVk->GetSampler());
					}
				}

				if (binding.DescriptorType == EDescriptorType::DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)
				{
					heapInfo.AccelerationStructureDescriptorCount += binding.DescriptorCount;
				}
				else if (binding.DescriptorType == EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER)
				{
					heapInfo.ConstantBufferDescriptorCount += binding.DescriptorCount;
				}
				else if (binding.DescriptorType == EDescriptorType::DESCRIPTOR_TYPE_SAMPLER)
				{
					heapInfo.SamplerDescriptorCount += binding.DescriptorCount;
				}
				else if (binding.DescriptorType == EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER)
				{
					heapInfo.TextureCombinedSamplerDescriptorCount += binding.DescriptorCount;
				}
				else if (binding.DescriptorType == EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_TEXTURE)
				{
					heapInfo.TextureDescriptorCount += binding.DescriptorCount;
				}
				else if (binding.DescriptorType == EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER)
				{
					heapInfo.UnorderedAccessBufferDescriptorCount += binding.DescriptorCount;
				}
				else if (binding.DescriptorType == EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_TEXTURE)
				{
					heapInfo.UnorderedAccessTextureDescriptorCount += binding.DescriptorCount;
				}

				bindingVk.descriptorType		= ConvertDescriptorType(binding.DescriptorType);
				bindingVk.binding				= binding.Binding;
				bindingVk.descriptorCount		= binding.DescriptorCount;
				bindingVk.pImmutableSamplers	= binding.ImmutableSamplers.empty() ? nullptr : immutableSamplers.data();
				bindingVk.stageFlags			= ConvertShaderStageMask(binding.ShaderStageMask);
				
				layoutBindings.emplace_back(bindingVk);
				m_DescriptorCounts.emplace_back(heapInfo);
			}

			m_DescriptorSetBindings.push_back({ descriptorSetLayout.DescriptorBindings });

			VkDescriptorSetLayoutCreateInfo createInfo = { };
			createInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.pNext		= nullptr;
			createInfo.flags		= 0;
			createInfo.pBindings	= layoutBindings.data();
			createInfo.bindingCount	= static_cast<uint32>(layoutBindings.size());

			VkResult result = vkCreateDescriptorSetLayout(m_pDevice->Device, &createInfo, nullptr, &layout);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[PipelineLayoutVK]: Failed to create DescriptorSetLayout");
				return false;
			}
			else
			{
				D_LOG_MESSAGE("[PipelineLayoutVK]: Created DescriptorSetLayout");
				
				m_DescriptorSetLayouts.emplace_back(layout);
				layoutBindings.clear();
			}
		}
		
#ifdef LAMBDA_DEVELOPMENT
		// Check limits
		DescriptorHeapInfo totalCount = {};
		for (DescriptorHeapInfo& heapInfo : m_DescriptorCounts)
		{
			totalCount += heapInfo;
		}
		
		VkPhysicalDeviceLimits limits = m_pDevice->GetDeviceLimits();
		VALIDATE(totalCount.UnorderedAccessTextureDescriptorCount	< limits.maxPerStageDescriptorStorageImages);
		VALIDATE(totalCount.UnorderedAccessBufferDescriptorCount	< limits.maxPerStageDescriptorStorageBuffers);
		VALIDATE(totalCount.ConstantBufferDescriptorCount			< limits.maxPerStageDescriptorUniformBuffers);
		
		const uint32 totalSamplerDescriptorCount = totalCount.TextureCombinedSamplerDescriptorCount + totalCount.SamplerDescriptorCount;
		VALIDATE(totalSamplerDescriptorCount < limits.maxPerStageDescriptorSamplers);

		const uint32 totalTextureDescriptorCount = totalCount.TextureCombinedSamplerDescriptorCount + totalCount.TextureDescriptorCount;
		VALIDATE(totalTextureDescriptorCount < limits.maxPerStageDescriptorSampledImages);
#endif
		
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
		pipelineLayoutCreateInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext					= nullptr;
		pipelineLayoutCreateInfo.flags					= 0;
		pipelineLayoutCreateInfo.pSetLayouts			= m_DescriptorSetLayouts.data();
		pipelineLayoutCreateInfo.setLayoutCount			= static_cast<uint32>(m_DescriptorSetLayouts.size());
		pipelineLayoutCreateInfo.pPushConstantRanges	= pushConstants.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32>(pushConstants.size());

		VkResult result = vkCreatePipelineLayout(m_pDevice->Device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);
		if (result != VK_SUCCESS)
		{
			if (!pDesc->DebugName.empty())
			{
				LOG_VULKAN_ERROR(result, "[PipelineLayoutVK]: Failed to create PipelineLayout \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[PipelineLayoutVK]: Failed to create PipelineLayout");
			}

			return false;
		}
		else
		{
			m_Desc = *pDesc;
			SetName(pDesc->DebugName);

			if (!pDesc->DebugName.empty())
			{
				D_LOG_MESSAGE("[PipelineLayoutVK]: Created PipelineLayout \"%s\"", pDesc->DebugName.c_str());
			}
			else
			{
				D_LOG_MESSAGE("[PipelineLayoutVK]: Created PipelineLayout");
			}
		}

		return true;
	}

	void PipelineLayoutVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_PipelineLayout), VK_OBJECT_TYPE_PIPELINE_LAYOUT);
		m_Desc.DebugName = debugName;

		for (VkDescriptorSetLayout& descriptorSetLayout : m_DescriptorSetLayouts)
		{
			m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(descriptorSetLayout), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT);
		}
	}
}
