#include "Log/Log.h"

#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	PipelineLayoutVK::PipelineLayoutVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
		, m_DescriptorCounts()
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
			pushConstants.EmplaceBack(constantRangeVk);
		}

		// DescriptorSetLayouts
		TArray<VkSampler> immutableSamplers;
		TArray<VkDescriptorSetLayoutBinding> layoutBindings;
		TArray<VkDescriptorBindingFlags> layoutBindingFlags;
		for (const DescriptorSetLayoutDesc& descriptorSetLayout : pDesc->DescriptorSetLayouts)
		{
			VkDescriptorSetLayout layout = VK_NULL_HANDLE;
			uint32 immutableSamplerOffset = 0;

			// Allocate enough size in immutableSamplers so that it does no get resized
			// Since it only has to be alive until the iteration of this loop we can realloc each iteration if needed
			// This is fine since it is unlikely that we will use so many descriptor sets that this gets noticable
			uint32 requiredSize = 0;
			for (const DescriptorBindingDesc& binding : descriptorSetLayout.DescriptorBindings)
			{
				requiredSize += binding.ImmutableSamplers.GetSize();
			}

			if (immutableSamplers.GetSize() < requiredSize)
			{
				immutableSamplers.Reserve(requiredSize);
			}

			// Bindings for each descriptorsetlayout
			for (const DescriptorBindingDesc& binding : descriptorSetLayout.DescriptorBindings)
			{
				VkFlags bindingFlags = 0;
				if (binding.Flags & FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND)
				{
					bindingFlags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
				}

				layoutBindingFlags.PushBack(bindingFlags);

				DescriptorHeapInfo heapInfo;
				VkDescriptorSetLayoutBinding bindingVk = { };

				// Immutable Samplers for each binding
				if (!binding.ImmutableSamplers.IsEmpty())
				{
					for (uint32 samplerIndex = 0; samplerIndex < binding.DescriptorCount; samplerIndex++)
					{
						// Store samplers to make sure that they are NOT released before we destroy the pipelinelayout
						const SamplerVK* pSamplerVk = reinterpret_cast<const SamplerVK*>(binding.ImmutableSamplers[samplerIndex].Get());
						m_ImmutableSamplers.EmplaceBack(binding.ImmutableSamplers[samplerIndex]);
						immutableSamplers.EmplaceBack(pSamplerVk->GetSampler());
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
				bindingVk.pImmutableSamplers	= binding.ImmutableSamplers.IsEmpty() ? nullptr : (immutableSamplers.GetData() + immutableSamplerOffset);
				bindingVk.stageFlags			= ConvertShaderStageMask(binding.ShaderStageMask);
				
				immutableSamplerOffset += binding.ImmutableSamplers.GetSize();

				layoutBindings.EmplaceBack(bindingVk);
				m_DescriptorCounts.EmplaceBack(heapInfo);
			}

			m_DescriptorSetBindings.PushBack({ descriptorSetLayout.DescriptorBindings });

			VkDescriptorSetLayoutBindingFlagsCreateInfo descriptorSetLayoutBindingFlags = {};
			descriptorSetLayoutBindingFlags.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
			descriptorSetLayoutBindingFlags.pNext			= nullptr;
			descriptorSetLayoutBindingFlags.bindingCount	= static_cast<uint32>(layoutBindingFlags.GetSize());
			descriptorSetLayoutBindingFlags.pBindingFlags	= layoutBindingFlags.GetData();

			VkDescriptorSetLayoutCreateInfo createInfo = { };
			createInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			createInfo.pNext		= &descriptorSetLayoutBindingFlags;
			createInfo.flags		= 0;
			createInfo.pBindings	= layoutBindings.GetData();
			createInfo.bindingCount	= static_cast<uint32>(layoutBindings.GetSize());

			if (descriptorSetLayout.DescriptorSetLayoutFlags & FDescriptorSetLayoutsFlag::DESCRIPTOR_SET_LAYOUT_FLAG_PUSH_DESCRIPTOR)
			{
				createInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			}
			if (descriptorSetLayout.DescriptorSetLayoutFlags & FDescriptorSetLayoutsFlag::DESCRIPTOR_SET_LAYOUT_FLAG_UPDATE_AFTER_BIND_POOL)
			{
				createInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
			}

			VkResult result = vkCreateDescriptorSetLayout(m_pDevice->Device, &createInfo, nullptr, &layout);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[PipelineLayoutVK]: Failed to create DescriptorSetLayout");
				return false;
			}
			else
			{
				D_LOG_MESSAGE("[PipelineLayoutVK]: Created DescriptorSetLayout");
				
				m_DescriptorSetLayouts.EmplaceBack(layout);
				layoutBindings.Clear();
				layoutBindingFlags.Clear();
				immutableSamplers.Clear();
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
		pipelineLayoutCreateInfo.pSetLayouts			= m_DescriptorSetLayouts.GetData();
		pipelineLayoutCreateInfo.setLayoutCount			= static_cast<uint32>(m_DescriptorSetLayouts.GetSize());
		pipelineLayoutCreateInfo.pPushConstantRanges	= pushConstants.GetData();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32>(pushConstants.GetSize());

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
