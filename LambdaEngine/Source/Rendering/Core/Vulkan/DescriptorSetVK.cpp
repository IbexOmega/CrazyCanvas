#include "Log/Log.h"

#include "Containers/TArray.h"

#include "Rendering/Core/Vulkan/DescriptorSetVK.h"
#include "Rendering/Core/Vulkan/DescriptorHeapVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/AccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/SamplerVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/PipelineLayoutVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	DescriptorSetVK::DescriptorSetVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
		, m_Bindings()
	{
	}

	DescriptorSetVK::~DescriptorSetVK()
	{
		if (m_DescriptorHeap)
		{
			m_DescriptorHeap->FreeDescriptorSet(m_DescriptorSet);
		}
	}

	bool DescriptorSetVK::Init(const String& debugName, const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, DescriptorHeap* pDescriptorHeap)
	{
		m_DebugName = debugName;

		DescriptorHeapVK* pVkDescriptorHeap = reinterpret_cast<DescriptorHeapVK*>(pDescriptorHeap);
		m_DescriptorSet = pVkDescriptorHeap->AllocateDescriptorSet(pPipelineLayout, descriptorLayoutIndex);
		if (m_DescriptorSet != VK_NULL_HANDLE)
		{
			SetName(debugName);

			const PipelineLayoutVK* pPipelineLayoutVk = reinterpret_cast<const PipelineLayoutVK*>(pPipelineLayout);
			m_Bindings = pPipelineLayoutVk->GetDescriptorBindings(descriptorLayoutIndex);
			m_BindingDescriptorCount.Resize(m_Bindings.GetSize());
			m_DescriptorHeap = pVkDescriptorHeap;
			m_DescriptorHeap->AddRef();
			return true;
		}
		else
		{
			return false;
		}
	}

	void DescriptorSetVK::SetBindingDescriptorCount(uint32 binding, uint32 count)
	{
		VALIDATE(binding < m_BindingDescriptorCount.GetSize());

		m_BindingDescriptorCount[binding] = count;
	}

	void DescriptorSetVK::SetName(const String& debugName)
	{
		m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_DescriptorSet), VK_OBJECT_TYPE_DESCRIPTOR_SET);
	}

	void DescriptorSetVK::WriteTextureDescriptors(const TextureView* const* ppTextures, const Sampler* const* ppSamplers, ETextureState textureState, uint32 firstBinding, uint32 descriptorCount, EDescriptorType descriptorType)
	{
		VALIDATE(ppTextures != nullptr);
		VALIDATE(firstBinding < m_Bindings.GetSize());

		const TextureViewVK* const* ppVkTextureViews	= reinterpret_cast<const TextureViewVK* const*>(ppTextures);
		const SamplerVK* const*		ppVkSamplers		= reinterpret_cast<const SamplerVK* const*>(ppSamplers);

		VkDescriptorType descriptorTypeVk = ConvertDescriptorType(descriptorType);
		
		VkImageLayout imageLayout = ConvertTextureState(textureState);

		TArray<VkDescriptorImageInfo> imageInfos(descriptorCount);
		for (uint32_t i = 0; i < descriptorCount; i++)
		{
			VkDescriptorImageInfo& imageInfo = imageInfos[i];
			imageInfo.imageLayout = imageLayout;

			VALIDATE(ppVkTextureViews[i] != nullptr);
			imageInfo.imageView	= ppVkTextureViews[i]->GetImageView();
			
			if (descriptorTypeVk == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				if (ppVkSamplers != nullptr)
				{
					VALIDATE(ppVkSamplers[i]	!= nullptr);
					imageInfo.sampler = ppVkSamplers[i]->GetSampler();
				}
				else
				{
					imageInfo.sampler = VK_NULL_HANDLE;
				}
			}
		}

		m_BindingDescriptorCount[firstBinding] = descriptorCount;

		VkWriteDescriptorSet descriptorImageWrite = {};
		descriptorImageWrite.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorImageWrite.dstSet				= m_DescriptorSet;
		descriptorImageWrite.dstBinding			= firstBinding;
		descriptorImageWrite.dstArrayElement	= 0;
		descriptorImageWrite.descriptorType		= descriptorTypeVk;
		descriptorImageWrite.pBufferInfo		= nullptr;
		descriptorImageWrite.descriptorCount	= uint32_t(imageInfos.GetSize());
		descriptorImageWrite.pImageInfo			= imageInfos.GetData();
		descriptorImageWrite.pTexelBufferView	= nullptr;

		vkUpdateDescriptorSets(m_pDevice->Device, 1, &descriptorImageWrite, 0, nullptr);
	}

	void DescriptorSetVK::WriteBufferDescriptors(const Buffer* const* ppBuffers, const uint64* pOffsets, const uint64* pSizes, uint32 firstBinding, uint32 descriptorCount, EDescriptorType descriptorType)
	{
		VALIDATE(ppBuffers	!= nullptr);
		VALIDATE(pOffsets	!= nullptr);
		VALIDATE(pSizes		!= nullptr);

		const BufferVK* const*	ppVkBuffers			= reinterpret_cast<const BufferVK* const*>(ppBuffers);
		VkDescriptorType		descriptorTypeVk	= ConvertDescriptorType(descriptorType);

		TArray<VkDescriptorBufferInfo> bufferInfos(descriptorCount);
		for (uint32_t i = 0; i < descriptorCount; i++)
		{
			VkDescriptorBufferInfo& bufferInfo = bufferInfos[i];
			bufferInfo.offset		= pOffsets[i];
			bufferInfo.range		= pSizes[i];

			VALIDATE(ppVkBuffers[i] != nullptr);
			bufferInfo.buffer	= ppVkBuffers[i]->GetBuffer();
		}

		m_BindingDescriptorCount[firstBinding] = descriptorCount;

		VkWriteDescriptorSet descriptorBufferWrite = {};
		descriptorBufferWrite.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorBufferWrite.dstSet			= m_DescriptorSet;
		descriptorBufferWrite.dstBinding		= firstBinding;
		descriptorBufferWrite.dstArrayElement	= 0;
		descriptorBufferWrite.descriptorType	= descriptorTypeVk;
		descriptorBufferWrite.descriptorCount	= uint32_t(bufferInfos.GetSize());
		descriptorBufferWrite.pBufferInfo		= bufferInfos.GetData();
		descriptorBufferWrite.pImageInfo		= nullptr;
		descriptorBufferWrite.pTexelBufferView	= nullptr;

		vkUpdateDescriptorSets(m_pDevice->Device, 1, &descriptorBufferWrite, 0, nullptr);
	}

	void DescriptorSetVK::WriteAccelerationStructureDescriptors(const AccelerationStructure* const * ppAccelerationStructures, uint32 firstBinding, uint32 descriptorCount)
	{
		VALIDATE(ppAccelerationStructures != nullptr);
		
		TArray<VkAccelerationStructureKHR> accelerationStructures(descriptorCount);
		for (uint32_t i = 0; i < descriptorCount; i++)
		{
			const AccelerationStructureVK* pAccelerationStructureVk = reinterpret_cast<const AccelerationStructureVK*>(ppAccelerationStructures[i]);

			VALIDATE(pAccelerationStructureVk != nullptr);
			accelerationStructures[i] = pAccelerationStructureVk->GetAccelerationStructure();
		}
		
		m_BindingDescriptorCount[firstBinding] = 1;

		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = {};
		descriptorAccelerationStructureInfo.sType                       = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount  = descriptorCount;
		descriptorAccelerationStructureInfo.pAccelerationStructures     = accelerationStructures.GetData();

		VkWriteDescriptorSet accelerationStructureWrite = {};
		accelerationStructureWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		accelerationStructureWrite.pNext            = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet           = m_DescriptorSet;
		accelerationStructureWrite.dstBinding       = firstBinding;
		accelerationStructureWrite.descriptorCount  = 1;
		accelerationStructureWrite.descriptorType   = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		accelerationStructureWrite.pBufferInfo      = nullptr;
		accelerationStructureWrite.pImageInfo       = nullptr;
		accelerationStructureWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_pDevice->Device, 1, &accelerationStructureWrite, 0, nullptr);
	}

	DescriptorHeap* DescriptorSetVK::GetHeap()
	{
		return m_DescriptorHeap.GetAndAddRef();
	}
}
