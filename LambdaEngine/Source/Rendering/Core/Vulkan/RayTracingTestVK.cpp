#include "Rendering/Core/Vulkan/RayTracingTestVK.h"
#include "Rendering/Core/Vulkan/CommandAllocatorVK.h"
#include "Rendering/Core/Vulkan/CommandListVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/ShaderVK.h"
#include "Rendering/Core/Vulkan/TextureViewVK.h"
#include "Rendering/Core/Vulkan/FenceVK.h"

#include "Rendering/RenderSystem.h"

#include "Resources/ResourceManager.h"

#include "Math/Math.h"

#include "Log/Log.h"

#include <array>

#define INDEX_RAYGEN 0
#define INDEX_CLOSEST_HIT 1
#define INDEX_MISS 2

namespace LambdaEngine
{
	AccelerationStructureVK*		RayTracingTestVK::s_pTLAS = nullptr;
	AccelerationStructureVK*		RayTracingTestVK::s_pBLAS = nullptr;
	
	BufferVK*						RayTracingTestVK::s_pBLASSerializedBuffer = nullptr;
	BufferVK*						RayTracingTestVK::s_pTLASSerializedBuffer = nullptr;
	
	IFence*							RayTracingTestVK::s_pFence		= nullptr;
	uint64							RayTracingTestVK::s_SignalValue	= 1;

	ITextureView*					RayTracingTestVK::s_ppTextureViews[3];

	CommandAllocatorVK*				RayTracingTestVK::s_pGraphicsPreCommandAllocators[3];
	CommandAllocatorVK*				RayTracingTestVK::s_pGraphicsPostCommandAllocators[3];
	CommandAllocatorVK*				RayTracingTestVK::s_pComputeCommandAllocators[3];
	
	CommandListVK*					RayTracingTestVK::s_pGraphicsPreCommandLists[3];
	CommandListVK*					RayTracingTestVK::s_pGraphicsPostCommandLists[3];
	CommandListVK*					RayTracingTestVK::s_pComputeCommandLists[3];
	
	VkPipeline						RayTracingTestVK::s_Pipeline			= VK_NULL_HANDLE;
	VkPipelineLayout				RayTracingTestVK::s_PipelineLayout		= VK_NULL_HANDLE;
	VkDescriptorSetLayout			RayTracingTestVK::s_DescriptorSetLayout	= VK_NULL_HANDLE;

	BufferVK*						RayTracingTestVK::s_pSBT = nullptr;

	VkDescriptorPool				RayTracingTestVK::s_DescriptorPool		= VK_NULL_HANDLE;
	VkDescriptorSet					RayTracingTestVK::s_DescriptorSets[3];


	void RayTracingTestVK::InitCommandLists()
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		for (uint32 i = 0; i < 3; i++)
		{
			s_pGraphicsPreCommandAllocators[i] = reinterpret_cast<CommandAllocatorVK*>(pGraphicsDeviceVk->CreateCommandAllocator("RT Testing Graphics Pre Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS));
			s_pGraphicsPostCommandAllocators[i] = reinterpret_cast<CommandAllocatorVK*>(pGraphicsDeviceVk->CreateCommandAllocator("RT Testing Graphics Post Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS));
			s_pComputeCommandAllocators[i] = reinterpret_cast<CommandAllocatorVK*>(pGraphicsDeviceVk->CreateCommandAllocator("RT Testing Compute Command Allocator", ECommandQueueType::COMMAND_QUEUE_COMPUTE));

			CommandListDesc graphicsPreCommandListDesc = {};
			graphicsPreCommandListDesc.pName			= "RT Testing Graphics Pre Command List";
			graphicsPreCommandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			graphicsPreCommandListDesc.CommandListType = ECommandListType::COMMAND_LIST_PRIMARY;

			CommandListDesc graphicsPostCommandListDesc = {};
			graphicsPostCommandListDesc.pName			= "RT Testing Graphics Post Command List";
			graphicsPostCommandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			graphicsPostCommandListDesc.CommandListType = ECommandListType::COMMAND_LIST_PRIMARY;

			CommandListDesc computeCommandListDesc = {};
			computeCommandListDesc.pName			= "RT Testing Compute Command List";
			computeCommandListDesc.Flags			= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;
			computeCommandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_PRIMARY;

			s_pGraphicsPreCommandLists[i] = reinterpret_cast<CommandListVK*>(pGraphicsDeviceVk->CreateCommandList(s_pGraphicsPreCommandAllocators[i], &graphicsPreCommandListDesc));
			s_pGraphicsPostCommandLists[i] = reinterpret_cast<CommandListVK*>(pGraphicsDeviceVk->CreateCommandList(s_pGraphicsPostCommandAllocators[i], &graphicsPostCommandListDesc));
			s_pComputeCommandLists[i] = reinterpret_cast<CommandListVK*>(pGraphicsDeviceVk->CreateCommandList(s_pComputeCommandAllocators[i], &computeCommandListDesc));
		}
	}

	void RayTracingTestVK::InitRenderer(ITextureView** ppBackBufferTextureViews, GUID_Lambda raygenShader, GUID_Lambda closestHitShader, GUID_Lambda missShader)
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		for (uint32 i = 0; i < 3; i++)
		{
			s_ppTextureViews[i] = ppBackBufferTextureViews[i];
		}

		VkResult result;

		VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
		acceleration_structure_layout_binding.binding         = 0;
		acceleration_structure_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		acceleration_structure_layout_binding.descriptorCount = 1;
		acceleration_structure_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		VkDescriptorSetLayoutBinding result_image_layout_binding{};
		result_image_layout_binding.binding         = 1;
		result_image_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		result_image_layout_binding.descriptorCount = 1;
		result_image_layout_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		//VkDescriptorSetLayoutBinding uniform_buffer_binding{};
		//uniform_buffer_binding.binding         = 2;
		//uniform_buffer_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		//uniform_buffer_binding.descriptorCount = 1;
		//uniform_buffer_binding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		std::vector<VkDescriptorSetLayoutBinding> bindings({acceleration_structure_layout_binding,
															result_image_layout_binding,
															/*uniform_buffer_binding*/});

		VkDescriptorSetLayoutCreateInfo layout_info{};
		layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
		layout_info.pBindings    = bindings.data();

		result = vkCreateDescriptorSetLayout(pGraphicsDeviceVk->Device, &layout_info, nullptr, &s_DescriptorSetLayout);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("INTE BRA");
		}

		VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
		pipeline_layout_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts    = &s_DescriptorSetLayout;

		result = vkCreatePipelineLayout(pGraphicsDeviceVk->Device, &pipeline_layout_create_info, nullptr, &s_PipelineLayout);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("INTE BRA");
		}

		ShaderVK* pRaygenShader		= reinterpret_cast<ShaderVK*>(ResourceManager::GetShader(raygenShader));
		ShaderVK* pClosestHitShader	= reinterpret_cast<ShaderVK*>(ResourceManager::GetShader(closestHitShader));
		ShaderVK* pMissShader		= reinterpret_cast<ShaderVK*>(ResourceManager::GetShader(missShader));

		std::array<VkPipelineShaderStageCreateInfo, 3> shaderCreateInfos;
		std::array<VkSpecializationInfo, 3> shaderSpecializationInfos;
		std::array<std::vector<VkSpecializationMapEntry>, 3> shaderSpecializationMapEntries;

		pRaygenShader->FillSpecializationInfo(shaderSpecializationInfos[INDEX_RAYGEN], shaderSpecializationMapEntries[INDEX_RAYGEN]);
		pRaygenShader->FillShaderStageInfo(shaderCreateInfos[INDEX_RAYGEN], &shaderSpecializationInfos[INDEX_RAYGEN]);

		pClosestHitShader->FillSpecializationInfo(shaderSpecializationInfos[INDEX_CLOSEST_HIT], shaderSpecializationMapEntries[INDEX_CLOSEST_HIT]);
		pClosestHitShader->FillShaderStageInfo(shaderCreateInfos[INDEX_CLOSEST_HIT], &shaderSpecializationInfos[INDEX_CLOSEST_HIT]);

		pMissShader->FillSpecializationInfo(shaderSpecializationInfos[INDEX_MISS], shaderSpecializationMapEntries[INDEX_MISS]);
		pMissShader->FillShaderStageInfo(shaderCreateInfos[INDEX_MISS], &shaderSpecializationInfos[INDEX_MISS]);

		std::array<VkPipelineShaderStageCreateInfo, 3> shader_stages;
		shader_stages[INDEX_RAYGEN]      = shaderCreateInfos[INDEX_RAYGEN];
		shader_stages[INDEX_MISS]        = shaderCreateInfos[INDEX_MISS];
		shader_stages[INDEX_CLOSEST_HIT] = shaderCreateInfos[INDEX_CLOSEST_HIT];

		/*
			Setup ray tracing shader groups
		*/
		std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> groups{};
		for (auto &group : groups)
		{
			// Init all groups with some default values
			group.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			group.generalShader      = VK_SHADER_UNUSED_KHR;
			group.closestHitShader   = VK_SHADER_UNUSED_KHR;
			group.anyHitShader       = VK_SHADER_UNUSED_KHR;
			group.intersectionShader = VK_SHADER_UNUSED_KHR;
		}

		// Links shaders and types to ray tracing shader groups
		groups[INDEX_RAYGEN].type                  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groups[INDEX_RAYGEN].generalShader         = INDEX_RAYGEN;
		groups[INDEX_MISS].type                    = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groups[INDEX_MISS].generalShader           = INDEX_MISS;
		groups[INDEX_CLOSEST_HIT].type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		groups[INDEX_CLOSEST_HIT].closestHitShader = INDEX_CLOSEST_HIT;

		VkRayTracingPipelineCreateInfoKHR raytracing_pipeline_create_info{};
		raytracing_pipeline_create_info.sType             = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		raytracing_pipeline_create_info.stageCount        = static_cast<uint32_t>(shader_stages.size());
		raytracing_pipeline_create_info.pStages           = shader_stages.data();
		raytracing_pipeline_create_info.groupCount        = static_cast<uint32_t>(groups.size());
		raytracing_pipeline_create_info.pGroups           = groups.data();
		raytracing_pipeline_create_info.maxRecursionDepth = 1;
		raytracing_pipeline_create_info.layout            = s_PipelineLayout;
		raytracing_pipeline_create_info.libraries.sType   = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;

		result = pGraphicsDeviceVk->vkCreateRayTracingPipelinesKHR(pGraphicsDeviceVk->Device, VK_NULL_HANDLE, 1, &raytracing_pipeline_create_info, nullptr, &s_Pipeline);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("INTE BRA");
		}

		uint32 shaderGroupHandleSize = pGraphicsDeviceVk->RayTracingProperties.shaderGroupHandleSize;
		uint32 sbtSize = shaderGroupHandleSize * 3;

		BufferDesc shaderHandleStorageDesc = {};
		shaderHandleStorageDesc.pName			= "Shader Handle Storage";
		shaderHandleStorageDesc.Flags			= BUFFER_FLAG_COPY_SRC;
		shaderHandleStorageDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		shaderHandleStorageDesc.SizeInBytes		= sbtSize;

		s_pSBT = reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&shaderHandleStorageDesc, nullptr));

		void* pMapped = s_pSBT->Map();
        
        result = pGraphicsDeviceVk->vkGetRayTracingShaderGroupHandlesKHR(pGraphicsDeviceVk->Device, s_Pipeline, 0, 3, sbtSize, pMapped);
		if (result!= VK_SUCCESS)
		{
			LOG_ERROR("INTE BRA");
		}

		s_pSBT->Unmap();

		//Create Descriptor Sets
		{
			std::vector<VkDescriptorPoolSize> pool_sizes = {
				{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};
			VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
			descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
			descriptor_pool_create_info.pPoolSizes    = pool_sizes.data();
			descriptor_pool_create_info.maxSets       = 3;
			
			result = vkCreateDescriptorPool(pGraphicsDeviceVk->Device, &descriptor_pool_create_info, nullptr, &s_DescriptorPool);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("INTE BRA");
			}

			VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
			descriptor_set_allocate_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptor_set_allocate_info.descriptorPool     = s_DescriptorPool;
			descriptor_set_allocate_info.pSetLayouts        = &s_DescriptorSetLayout;
			descriptor_set_allocate_info.descriptorSetCount = 1;

			for (uint32 i = 0; i < 3; i++)
			{
				result = vkAllocateDescriptorSets(pGraphicsDeviceVk->Device, &descriptor_set_allocate_info, &s_DescriptorSets[i]);
				if (result != VK_SUCCESS)
				{
					LOG_ERROR("INTE BRA");
				}

				VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
				descriptor_acceleration_structure_info.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				descriptor_acceleration_structure_info.accelerationStructureCount = 1;
				descriptor_acceleration_structure_info.pAccelerationStructures    = &s_pTLAS->m_AccelerationStructure;

				VkWriteDescriptorSet acceleration_structure_write{};
				acceleration_structure_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				// The specialized acceleration structure descriptor has to be chained
				acceleration_structure_write.pNext           = &descriptor_acceleration_structure_info;
				acceleration_structure_write.dstSet          = s_DescriptorSets[i];
				acceleration_structure_write.dstBinding      = 0;
				acceleration_structure_write.descriptorCount = 1;
				acceleration_structure_write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

				VkDescriptorImageInfo image_descriptor{};
				image_descriptor.imageView   = reinterpret_cast<TextureViewVK**>(ppBackBufferTextureViews)[i]->GetImageView();
				image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

				/*VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*ubo);*/

				VkWriteDescriptorSet result_image_write = {};
				result_image_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				result_image_write.dstSet          = s_DescriptorSets[i];
				result_image_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				result_image_write.dstBinding      = 1;
				result_image_write.pImageInfo      = &image_descriptor;
				result_image_write.descriptorCount = 1;

				std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
					acceleration_structure_write,
					result_image_write,
					/*uniform_buffer_write*/};
				vkUpdateDescriptorSets(pGraphicsDeviceVk->Device, static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, VK_NULL_HANDLE);
			}
		}

		FenceDesc fenceDesc = {};
		fenceDesc.pName			= "RT Testing Fence";
		fenceDesc.InitalValue	= 0;

		s_pFence = reinterpret_cast<FenceVK*>(pGraphicsDeviceVk->CreateFence(&fenceDesc));

		if (s_pFence == nullptr)
		{
			LOG_ERROR("Bajs");
		}
	}

	void RayTracingTestVK::CreateBLAS()
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());
		s_pBLAS = DBG_NEW AccelerationStructureVK(pGraphicsDeviceVk);

		VkResult result;

		VkAccelerationStructureCreateGeometryTypeInfoKHR acceleration_create_geometry_info{};
		acceleration_create_geometry_info.sType             = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		acceleration_create_geometry_info.geometryType      = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		acceleration_create_geometry_info.maxPrimitiveCount = 1;
		acceleration_create_geometry_info.indexType         = VK_INDEX_TYPE_UINT32;
		acceleration_create_geometry_info.maxVertexCount    = 3;
		acceleration_create_geometry_info.vertexFormat      = VK_FORMAT_R32G32B32_SFLOAT;
		acceleration_create_geometry_info.allowsTransforms  = VK_FALSE;

		VkAccelerationStructureCreateInfoKHR acceleration_create_info{};
		acceleration_create_info.sType            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		acceleration_create_info.type             = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		acceleration_create_info.flags            = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_create_info.maxGeometryCount = 1;
		acceleration_create_info.pGeometryInfos   = &acceleration_create_geometry_info;

		result = pGraphicsDeviceVk->vkCreateAccelerationStructureKHR(pGraphicsDeviceVk->Device, &acceleration_create_info, nullptr, &s_pBLAS->m_AccelerationStructure);
		if (result != VK_SUCCESS)
		{
			LOG_ERROR("BAJSKORV 1");
		}

		//Object Memory
		{
			VkMemoryRequirements2 memory_requirements_2{};
			memory_requirements_2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

			VkAccelerationStructureMemoryRequirementsInfoKHR acceleration_memory_requirements{};
			acceleration_memory_requirements.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
			acceleration_memory_requirements.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
			acceleration_memory_requirements.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
			acceleration_memory_requirements.accelerationStructure = s_pBLAS->m_AccelerationStructure;
			pGraphicsDeviceVk->vkGetAccelerationStructureMemoryRequirementsKHR(pGraphicsDeviceVk->Device, &acceleration_memory_requirements, &memory_requirements_2);

			VkMemoryRequirements memory_requirements = memory_requirements_2.memoryRequirements;

			VkMemoryAllocateInfo memory_allocate_info{};
			memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memory_allocate_info.allocationSize  = memory_requirements.size;
			memory_allocate_info.memoryTypeIndex = FindMemoryType(pGraphicsDeviceVk->PhysicalDevice, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			result = vkAllocateMemory(pGraphicsDeviceVk->Device, &memory_allocate_info, nullptr, &s_pBLAS->m_AccelerationStructureMemory);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("BAJSKORV 2");
			}

			VkBindAccelerationStructureMemoryInfoKHR bind_acceleration_memory_info{};
			bind_acceleration_memory_info.sType                 = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
			bind_acceleration_memory_info.accelerationStructure = s_pBLAS->m_AccelerationStructure;
			bind_acceleration_memory_info.memory                = s_pBLAS->m_AccelerationStructureMemory;

			result = pGraphicsDeviceVk->vkBindAccelerationStructureMemoryKHR(pGraphicsDeviceVk->Device, 1, &bind_acceleration_memory_info);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("BAJSKORV 3");
			}
		}

		//Scratch Memory
		{
			/*VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo = {};
			memoryRequirementsInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
			memoryRequirementsInfo.pNext					= nullptr;
			memoryRequirementsInfo.type						= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
			memoryRequirementsInfo.accelerationStructure	= pAccelerationStructureVk->m_AccelerationStructure;
			memoryRequirementsInfo.buildType				= VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

			VkMemoryRequirements2 memoryRequirements2 = {};
			memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
			memoryRequirements2.pNext = nullptr;

			pGraphicsDeviceVk->vkGetAccelerationStructureMemoryRequirementsKHR(pGraphicsDeviceVk->Device, &memoryRequirementsInfo, &memoryRequirements2);

			BufferDesc scratchBufferDesc = {};
			scratchBufferDesc.pName			= "Acceleration Structure Scratch Buffer";
			scratchBufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
			scratchBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_RAY_TRACING;
			scratchBufferDesc.SizeInBytes	= memoryRequirements2.memoryRequirements.size;

			pAccelerationStructureVk->m_pScratchBuffer = reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&scratchBufferDesc, nullptr));*/

			BufferVK* pScratchBuffer = DBG_NEW BufferVK(pGraphicsDeviceVk);
			s_pBLAS->m_pScratchBuffer = pScratchBuffer;

			VkMemoryRequirements2 memory_requirements_2{};
			memory_requirements_2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

			VkAccelerationStructureMemoryRequirementsInfoKHR acceleration_memory_requirements{};
			acceleration_memory_requirements.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
			acceleration_memory_requirements.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
			acceleration_memory_requirements.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
			acceleration_memory_requirements.accelerationStructure = s_pBLAS->m_AccelerationStructure;
			pGraphicsDeviceVk->vkGetAccelerationStructureMemoryRequirementsKHR(pGraphicsDeviceVk->Device, &acceleration_memory_requirements, &memory_requirements_2);

			VkBufferCreateInfo buffer_create_info{};
			buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_create_info.size        = memory_requirements_2.memoryRequirements.size;
			buffer_create_info.usage       = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			result = vkCreateBuffer(pGraphicsDeviceVk->Device, &buffer_create_info, nullptr, &pScratchBuffer->m_Buffer);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("MEGABAJSKORV 4");
			}

			VkMemoryRequirements memory_requirements{};
			vkGetBufferMemoryRequirements(pGraphicsDeviceVk->Device, pScratchBuffer->m_Buffer, &memory_requirements);

			VkMemoryAllocateFlagsInfo memory_allocate_flags_info{};
			memory_allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
			memory_allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

			VkMemoryAllocateInfo memory_allocate_info{};
			memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memory_allocate_info.pNext           = &memory_allocate_flags_info;
			memory_allocate_info.allocationSize  = memory_requirements.size;
			memory_allocate_info.memoryTypeIndex = FindMemoryType(pGraphicsDeviceVk->PhysicalDevice, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			result = vkAllocateMemory(pGraphicsDeviceVk->Device, &memory_allocate_info, nullptr, &pScratchBuffer->m_Memory);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("MEGABAJSKORV 5");
			}

			result = vkBindBufferMemory(pGraphicsDeviceVk->Device, pScratchBuffer->m_Buffer, pScratchBuffer->m_Memory, 0);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("MEGABAJSKORV 6");
			}

			VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
			buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			buffer_device_address_info.buffer = pScratchBuffer->m_Buffer;
			pScratchBuffer->m_DeviceAddress  = pGraphicsDeviceVk->vkGetBufferDeviceAddress(pGraphicsDeviceVk->Device, &buffer_device_address_info);
		}

		VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
		acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		acceleration_device_address_info.accelerationStructure = s_pBLAS->m_AccelerationStructure;

		s_pBLAS->m_AccelerationStructureDeviceAddress = pGraphicsDeviceVk->vkGetAccelerationStructureDeviceAddressKHR(pGraphicsDeviceVk->Device, &acceleration_device_address_info);
	}

	void RayTracingTestVK::BuildBLAS()
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		//VALIDATE(pBuildDesc->pVertexBuffer != nullptr);
		//VALIDATE(pBuildDesc->pIndexBuffer != nullptr);
		//VALIDATE(pBuildDesc->pTransformBuffer != nullptr);

		VkResult result;

		//const BufferVK* pVertexBufferVk		= reinterpret_cast<const BufferVK*>(pBuildDesc->pVertexBuffer);
		//const BufferVK* pIndexBufferVk		= reinterpret_cast<const BufferVK*>(pBuildDesc->pIndexBuffer);
		//const BufferVK* pTransformBufferVk	= reinterpret_cast<const BufferVK*>(pBuildDesc->pTransformBuffer);

		struct Vertex
		{
			float pos[3];
		};
		std::vector<Vertex> vertices = {
			{{1.0f, 1.0f, 0.0f}},
			{{-1.0f, 1.0f, 0.0f}},
			{{0.0f, -1.0f, 0.0f}} };
		std::vector<uint32_t> indices = { 0, 1, 2 };

		glm::mat3x4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));

		std::vector<glm::mat3x4> transforms = { scale };

		auto vertex_buffer_size = vertices.size() * sizeof(Vertex);
		auto index_buffer_size = indices.size() * sizeof(uint32_t);
		auto transform_buffer_size = transforms.size() * sizeof(glm::mat3x4);

		BufferDesc vertexBufferDesc = {};
		vertexBufferDesc.pName				= "Temp Vertex Buffer";
		vertexBufferDesc.MemoryType			= EMemoryType::MEMORY_CPU_VISIBLE;
		vertexBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_VERTEX_BUFFER;
		vertexBufferDesc.SizeInBytes		= vertex_buffer_size;

		BufferDesc indexBufferDesc = {};
		indexBufferDesc.pName				= "Temp Index Buffer";
		indexBufferDesc.MemoryType			= EMemoryType::MEMORY_CPU_VISIBLE;
		indexBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_INDEX_BUFFER;
		indexBufferDesc.SizeInBytes			= index_buffer_size;

		BufferDesc transformBufferDesc = {};
		transformBufferDesc.pName			= "Temp Transform Buffer";
		transformBufferDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		transformBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_NONE;
		transformBufferDesc.SizeInBytes		= transform_buffer_size;

		BufferVK* pTempVertexBuffer		= reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&vertexBufferDesc, nullptr));
		BufferVK* pTempIndexBuffer		= reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&indexBufferDesc, nullptr));
		BufferVK* pTempTransformBuffer	= reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&transformBufferDesc, nullptr));

		void* pMapped;

		pMapped = pTempVertexBuffer->Map();
		memcpy(pMapped, vertices.data(), vertex_buffer_size);
		pTempVertexBuffer->Unmap();

		pMapped = pTempIndexBuffer->Map();
		memcpy(pMapped, indices.data(), index_buffer_size);
		pTempIndexBuffer->Unmap();

		pMapped = pTempTransformBuffer->Map();
		memcpy(pMapped, transforms.data(), transform_buffer_size);
		pTempTransformBuffer->Unmap();

		VkDeviceOrHostAddressConstKHR vertex_data_device_address{};
		VkDeviceOrHostAddressConstKHR index_data_device_address{};
		VkDeviceOrHostAddressConstKHR transform_data_device_address{};

		vertex_data_device_address.deviceAddress		= pTempVertexBuffer->GetDeviceAdress();
		index_data_device_address.deviceAddress			= pTempIndexBuffer->GetDeviceAdress();
		transform_data_device_address.deviceAddress		= pTempTransformBuffer->GetDeviceAdress();

		VkAccelerationStructureGeometryKHR acceleration_geometry{};
		acceleration_geometry.sType												= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		acceleration_geometry.flags												= VK_GEOMETRY_OPAQUE_BIT_KHR;
		acceleration_geometry.geometryType										= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		acceleration_geometry.geometry.triangles.sType							= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		acceleration_geometry.geometry.triangles.vertexFormat					= VK_FORMAT_R32G32B32_SFLOAT;
		acceleration_geometry.geometry.triangles.vertexData.deviceAddress		= vertex_data_device_address.deviceAddress;
		acceleration_geometry.geometry.triangles.vertexStride					= sizeof(Vertex);
		acceleration_geometry.geometry.triangles.indexType						= VK_INDEX_TYPE_UINT32;
		acceleration_geometry.geometry.triangles.indexData.deviceAddress		= index_data_device_address.deviceAddress;
		//acceleration_geometry.geometry.triangles.transformData.deviceAddress	= transform_data_device_address.deviceAddress;

		std::vector<VkAccelerationStructureGeometryKHR> acceleration_geometries           = {acceleration_geometry};
		VkAccelerationStructureGeometryKHR *            acceleration_structure_geometries = acceleration_geometries.data();

		VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
		acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_build_geometry_info.update                    = VK_FALSE;
		acceleration_build_geometry_info.dstAccelerationStructure  = s_pBLAS->m_AccelerationStructure;
		acceleration_build_geometry_info.geometryArrayOfPointers   = VK_FALSE;
		acceleration_build_geometry_info.geometryCount             = 1;
		acceleration_build_geometry_info.ppGeometries              = &acceleration_structure_geometries;
		acceleration_build_geometry_info.scratchData.deviceAddress = s_pBLAS->m_pScratchBuffer->GetDeviceAdress();

		VkAccelerationStructureBuildOffsetInfoKHR acceleration_build_offset_info{};
		acceleration_build_offset_info.primitiveCount	= 1; //pBuildDesc->TriangleCount;
		acceleration_build_offset_info.primitiveOffset	= 0; //pBuildDesc->IndexBufferByteOffset;
		acceleration_build_offset_info.firstVertex		= 0; //pBuildDesc->FirstVertexIndex;
		acceleration_build_offset_info.transformOffset	= 0; //pBuildDesc->TransformByteOffset;

		std::vector<VkAccelerationStructureBuildOffsetInfoKHR*> acceleration_build_offsets = { &acceleration_build_offset_info };

		s_pComputeCommandAllocators[0]->Reset();
		s_pComputeCommandLists[0]->Begin(nullptr);

		pGraphicsDeviceVk->vkCmdBuildAccelerationStructureKHR(s_pComputeCommandLists[0]->GetCommandBuffer(), 1, &acceleration_build_geometry_info, acceleration_build_offsets.data());

		BufferDesc blasSerializedBufferDesc = {};
		blasSerializedBufferDesc.pName			= "BLAS Serialized";
		blasSerializedBufferDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		blasSerializedBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_COPY_DST;
		blasSerializedBufferDesc.SizeInBytes	= 100 * 1024 * 1024;

		s_pBLASSerializedBuffer = reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&blasSerializedBufferDesc, nullptr));

		VkCopyAccelerationStructureToMemoryInfoKHR blasCopyInfo = {};
		blasCopyInfo.sType				= VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
		blasCopyInfo.pNext				= nullptr;
		blasCopyInfo.src				= s_pBLAS->m_AccelerationStructure;
		blasCopyInfo.dst.deviceAddress	= s_pBLASSerializedBuffer->GetDeviceAdress();
		blasCopyInfo.mode				= VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR;

		pGraphicsDeviceVk->vkCmdCopyAccelerationStructureToMemoryKHR(s_pComputeCommandLists[0]->GetCommandBuffer(), &blasCopyInfo);

		s_pComputeCommandLists[0]->End();

		ICommandList* pCommandList = s_pComputeCommandLists[0];
		RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, nullptr, 0, nullptr, 0);
		RenderSystem::GetComputeQueue()->Flush();
	}

	void RayTracingTestVK::CreateTLAS()
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());
		s_pTLAS = DBG_NEW AccelerationStructureVK(pGraphicsDeviceVk);

		//VALIDATE(pBuildDesc->pInstanceBuffer != nullptr);

		VkResult result;

		//const BufferVK* pInstanceBufferVk = reinterpret_cast<const BufferVK*>(pBuildDesc->pInstanceBuffer);

		VkTransformMatrixKHR transform_matrix = {
		    1.0f, 0.0f, 0.0f, 0.0f,
		    0.0f, 1.0f, 0.0f, 0.0f,
		    0.0f, 0.0f, 1.0f, 0.0f};

		VkAccelerationStructureInstanceKHR instance{};
		instance.transform                              = transform_matrix;
		instance.instanceCustomIndex                    = 0;
		instance.mask                                   = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference         = s_pBLAS->GetHandle();																										//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXTodo: Is this right?

		std::vector<VkAccelerationStructureInstanceKHR> instances(1, instance);

		BufferDesc instanceBufferDesc = {};
		instanceBufferDesc.pName			= "Temp Instance Buffer";
		instanceBufferDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		instanceBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_NONE;
		instanceBufferDesc.SizeInBytes		= instances.size() * sizeof(VkAccelerationStructureInstanceKHR);

		BufferVK* pTempInstanceBuffer = reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&instanceBufferDesc, nullptr));

		void* pMapped;

		pMapped = pTempInstanceBuffer->Map();
		memcpy(pMapped, instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR));
		pTempInstanceBuffer->Unmap();

		VkAccelerationStructureCreateGeometryTypeInfoKHR acceleration_create_geometry_info{};
		acceleration_create_geometry_info.sType             = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		acceleration_create_geometry_info.geometryType      = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		acceleration_create_geometry_info.maxPrimitiveCount = 1;
		acceleration_create_geometry_info.allowsTransforms  = VK_FALSE;

		VkAccelerationStructureCreateInfoKHR acceleration_create_info{};
		acceleration_create_info.sType            = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		acceleration_create_info.type             = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		acceleration_create_info.flags            = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_create_info.maxGeometryCount = 1;
		acceleration_create_info.pGeometryInfos   = &acceleration_create_geometry_info;
		result = pGraphicsDeviceVk->vkCreateAccelerationStructureKHR(pGraphicsDeviceVk->Device, &acceleration_create_info, nullptr, &s_pTLAS->m_AccelerationStructure);

		if (result != VK_SUCCESS)
		{
			LOG_ERROR("MEGABAJSKORV 1");
		}

		// Bind object memory to the top level acceleration structure
		//Object Memory
		{
			VkMemoryRequirements2 memory_requirements_2{};
			memory_requirements_2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

			VkAccelerationStructureMemoryRequirementsInfoKHR acceleration_memory_requirements{};
			acceleration_memory_requirements.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
			acceleration_memory_requirements.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
			acceleration_memory_requirements.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
			acceleration_memory_requirements.accelerationStructure = s_pTLAS->m_AccelerationStructure;
			pGraphicsDeviceVk->vkGetAccelerationStructureMemoryRequirementsKHR(pGraphicsDeviceVk->Device, &acceleration_memory_requirements, &memory_requirements_2);

			VkMemoryRequirements memory_requirements = memory_requirements_2.memoryRequirements;

			VkMemoryAllocateInfo memory_allocate_info{};
			memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memory_allocate_info.allocationSize  = memory_requirements.size;
			memory_allocate_info.memoryTypeIndex = FindMemoryType(pGraphicsDeviceVk->PhysicalDevice, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			result = vkAllocateMemory(pGraphicsDeviceVk->Device, &memory_allocate_info, nullptr, &s_pTLAS->m_AccelerationStructureMemory);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("MEGABAJSKORV 2");
			}

			VkBindAccelerationStructureMemoryInfoKHR bind_acceleration_memory_info{};
			bind_acceleration_memory_info.sType                 = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
			bind_acceleration_memory_info.accelerationStructure = s_pTLAS->m_AccelerationStructure;
			bind_acceleration_memory_info.memory                = s_pTLAS->m_AccelerationStructureMemory;

			result = pGraphicsDeviceVk->vkBindAccelerationStructureMemoryKHR(pGraphicsDeviceVk->Device, 1, &bind_acceleration_memory_info);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("MEGABAJSKORV 3");
			}
		}

		VkDeviceOrHostAddressConstKHR instance_data_device_address{};
		instance_data_device_address.deviceAddress = pTempInstanceBuffer->GetDeviceAdress();

		VkAccelerationStructureGeometryKHR acceleration_geometry{};
		acceleration_geometry.sType                                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		acceleration_geometry.flags                                 = VK_GEOMETRY_OPAQUE_BIT_KHR;
		acceleration_geometry.geometryType                          = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		acceleration_geometry.geometry.instances.sType              = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		acceleration_geometry.geometry.instances.arrayOfPointers    = VK_FALSE;
		acceleration_geometry.geometry.instances.data.deviceAddress = instance_data_device_address.deviceAddress;

		std::vector<VkAccelerationStructureGeometryKHR> acceleration_geometries           = {acceleration_geometry};
		VkAccelerationStructureGeometryKHR *            acceleration_structure_geometries = acceleration_geometries.data();

		//Scratch Memory
		{
			/*VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo = {};
			memoryRequirementsInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
			memoryRequirementsInfo.pNext					= nullptr;
			memoryRequirementsInfo.type						= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
			memoryRequirementsInfo.accelerationStructure	= pAccelerationStructureVk->m_AccelerationStructure;
			memoryRequirementsInfo.buildType				= VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

			VkMemoryRequirements2 memoryRequirements2 = {};
			memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
			memoryRequirements2.pNext = nullptr;

			pGraphicsDeviceVk->vkGetAccelerationStructureMemoryRequirementsKHR(pGraphicsDeviceVk->Device, &memoryRequirementsInfo, &memoryRequirements2);

			BufferDesc scratchBufferDesc = {};
			scratchBufferDesc.pName			= "Acceleration Structure Scratch Buffer";
			scratchBufferDesc.MemoryType	= EMemoryType::MEMORY_GPU;
			scratchBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_RAY_TRACING;
			scratchBufferDesc.SizeInBytes	= memoryRequirements2.memoryRequirements.size;

			pAccelerationStructureVk->m_pScratchBuffer = reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&scratchBufferDesc, nullptr));*/

			BufferVK* pScratchBuffer = DBG_NEW BufferVK(pGraphicsDeviceVk);
			s_pTLAS->m_pScratchBuffer = pScratchBuffer;

			VkMemoryRequirements2 memory_requirements_2{};
			memory_requirements_2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

			VkAccelerationStructureMemoryRequirementsInfoKHR acceleration_memory_requirements{};
			acceleration_memory_requirements.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
			acceleration_memory_requirements.type                  = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
			acceleration_memory_requirements.buildType             = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
			acceleration_memory_requirements.accelerationStructure = s_pTLAS->m_AccelerationStructure;
			pGraphicsDeviceVk->vkGetAccelerationStructureMemoryRequirementsKHR(pGraphicsDeviceVk->Device, &acceleration_memory_requirements, &memory_requirements_2);

			VkBufferCreateInfo buffer_create_info{};
			buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_create_info.size        = memory_requirements_2.memoryRequirements.size;
			buffer_create_info.usage       = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			result = vkCreateBuffer(pGraphicsDeviceVk->Device, &buffer_create_info, nullptr, &pScratchBuffer->m_Buffer);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("MEGABAJSKORV 4");
			}

			VkMemoryRequirements memory_requirements{};
			vkGetBufferMemoryRequirements(pGraphicsDeviceVk->Device, pScratchBuffer->m_Buffer, &memory_requirements);

			VkMemoryAllocateFlagsInfo memory_allocate_flags_info{};
			memory_allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
			memory_allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

			VkMemoryAllocateInfo memory_allocate_info{};
			memory_allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memory_allocate_info.pNext           = &memory_allocate_flags_info;
			memory_allocate_info.allocationSize  = memory_requirements.size;
			memory_allocate_info.memoryTypeIndex = FindMemoryType(pGraphicsDeviceVk->PhysicalDevice, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			result = vkAllocateMemory(pGraphicsDeviceVk->Device, &memory_allocate_info, nullptr, &pScratchBuffer->m_Memory);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("MEGABAJSKORV 5");
			}

			result = vkBindBufferMemory(pGraphicsDeviceVk->Device, pScratchBuffer->m_Buffer, pScratchBuffer->m_Memory, 0);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("MEGABAJSKORV 6");
			}

			VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
			buffer_device_address_info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			buffer_device_address_info.buffer = pScratchBuffer->m_Buffer;
			pScratchBuffer->m_DeviceAddress  = pGraphicsDeviceVk->vkGetBufferDeviceAddress(pGraphicsDeviceVk->Device, &buffer_device_address_info);
		}

		VkAccelerationStructureBuildGeometryInfoKHR acceleration_build_geometry_info{};
		acceleration_build_geometry_info.sType                     = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		acceleration_build_geometry_info.type                      = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		acceleration_build_geometry_info.flags                     = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		acceleration_build_geometry_info.update                    = VK_FALSE;
		acceleration_build_geometry_info.srcAccelerationStructure  = VK_NULL_HANDLE;
		acceleration_build_geometry_info.dstAccelerationStructure  = s_pTLAS->m_AccelerationStructure;
		acceleration_build_geometry_info.geometryArrayOfPointers   = VK_FALSE;
		acceleration_build_geometry_info.geometryCount             = 1;
		acceleration_build_geometry_info.ppGeometries              = &acceleration_structure_geometries;
		acceleration_build_geometry_info.scratchData.deviceAddress = s_pTLAS->m_pScratchBuffer->GetDeviceAdress();

		VkAccelerationStructureBuildOffsetInfoKHR acceleration_build_offset_info{};
		acceleration_build_offset_info.primitiveCount                                       = 1;
		acceleration_build_offset_info.primitiveOffset                                      = 0x0;
		acceleration_build_offset_info.firstVertex                                          = 0;
		acceleration_build_offset_info.transformOffset                                      = 0x0;
		std::vector<VkAccelerationStructureBuildOffsetInfoKHR *> acceleration_build_offsets = {&acceleration_build_offset_info};

		s_pComputeCommandAllocators[0]->Reset();
		s_pComputeCommandLists[0]->Begin(nullptr);

		pGraphicsDeviceVk->vkCmdBuildAccelerationStructureKHR(s_pComputeCommandLists[0]->GetCommandBuffer(), 1, &acceleration_build_geometry_info, acceleration_build_offsets.data());

		VkAccelerationStructureDeviceAddressInfoKHR acceleration_device_address_info{};
		acceleration_device_address_info.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		acceleration_device_address_info.accelerationStructure = s_pTLAS->GetAccelerationStructure();

		s_pTLAS->m_AccelerationStructureDeviceAddress = pGraphicsDeviceVk->vkGetAccelerationStructureDeviceAddressKHR(pGraphicsDeviceVk->Device, &acceleration_device_address_info);

		BufferDesc tlasSerializedBufferDesc = {};
		tlasSerializedBufferDesc.pName			= "TLAS Serialized";
		tlasSerializedBufferDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		tlasSerializedBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_COPY_DST;
		tlasSerializedBufferDesc.SizeInBytes	= 100 * 1024 * 1024;

		s_pTLASSerializedBuffer = reinterpret_cast<BufferVK*>(pGraphicsDeviceVk->CreateBuffer(&tlasSerializedBufferDesc, nullptr));

		VkCopyAccelerationStructureToMemoryInfoKHR tlasCopyInfo = {};
		tlasCopyInfo.sType				= VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
		tlasCopyInfo.pNext				= nullptr;
		tlasCopyInfo.src				= s_pTLAS->m_AccelerationStructure;
		tlasCopyInfo.dst.deviceAddress	= s_pTLASSerializedBuffer->GetDeviceAdress();
		tlasCopyInfo.mode				= VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR;

		pGraphicsDeviceVk->vkCmdCopyAccelerationStructureToMemoryKHR(s_pComputeCommandLists[0]->GetCommandBuffer(), &tlasCopyInfo);

		s_pComputeCommandLists[0]->End();

		ICommandList* pCommandList = s_pComputeCommandLists[0];
		RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, nullptr, 0, nullptr, 0);
		RenderSystem::GetComputeQueue()->Flush();
	}

	void RayTracingTestVK::GetAndIncrementFence(IFence** ppFence, uint64* pSignalValue)
	{
		(*pSignalValue)	= s_SignalValue++;
		(*ppFence)		= s_pFence;
	}

	void RayTracingTestVK::Render(uint64 modFrameIndex, uint32 backBufferIndex)
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());

		s_pGraphicsPreCommandAllocators[modFrameIndex]->Reset();
		s_pGraphicsPostCommandAllocators[modFrameIndex]->Reset();
		s_pComputeCommandAllocators[modFrameIndex]->Reset();

		ICommandList* pGraphicsPreCommandList = reinterpret_cast<ICommandList*>(s_pGraphicsPreCommandLists[modFrameIndex]);
		ICommandList* pComputeCommandList = reinterpret_cast<ICommandList*>(s_pComputeCommandLists[modFrameIndex]);
		ICommandList* pGraphicsPostCommandList = reinterpret_cast<ICommandList*>(s_pGraphicsPostCommandLists[modFrameIndex]);

		PipelineTextureBarrierDesc fromGraphicsToComputeBarrier = {};
		fromGraphicsToComputeBarrier.pTexture				= s_ppTextureViews[backBufferIndex]->GetDesc().pTexture;
		fromGraphicsToComputeBarrier.StateBefore			= ETextureState::TEXTURE_STATE_DONT_CARE;
		fromGraphicsToComputeBarrier.StateAfter				= ETextureState::TEXTURE_STATE_GENERAL;
		fromGraphicsToComputeBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
		fromGraphicsToComputeBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_COMPUTE;
		fromGraphicsToComputeBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		fromGraphicsToComputeBarrier.DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		//fromGraphicsToComputeBarrier.TextureFlags			= FTextureFlags::TEXTURE_FLAG_UNORDERED_ACCESS;
		fromGraphicsToComputeBarrier.Miplevel				= 0;
		fromGraphicsToComputeBarrier.MiplevelCount			= 1;
		fromGraphicsToComputeBarrier.ArrayIndex				= 0;
		fromGraphicsToComputeBarrier.ArrayCount				= 1;

		PipelineTextureBarrierDesc fromComputeToGraphicsBarrier = {};
		fromComputeToGraphicsBarrier.pTexture				= s_ppTextureViews[backBufferIndex]->GetDesc().pTexture;
		fromComputeToGraphicsBarrier.StateBefore			= ETextureState::TEXTURE_STATE_GENERAL;
		fromComputeToGraphicsBarrier.StateAfter				= ETextureState::TEXTURE_STATE_PRESENT;
		fromComputeToGraphicsBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_COMPUTE;
		fromComputeToGraphicsBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_GRAPHICS;
		fromComputeToGraphicsBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		fromComputeToGraphicsBarrier.DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		//fromComputeToGraphicsBarrier.TextureFlags			= FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		fromComputeToGraphicsBarrier.Miplevel				= 0;
		fromComputeToGraphicsBarrier.MiplevelCount			= 1;
		fromComputeToGraphicsBarrier.ArrayIndex				= 0;
		fromComputeToGraphicsBarrier.ArrayCount				= 1;

		pGraphicsPreCommandList->Begin(nullptr);
		pGraphicsPreCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER, &fromGraphicsToComputeBarrier, 1);
		pGraphicsPreCommandList->End();
		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&pGraphicsPreCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, s_pFence, s_SignalValue - 1, s_pFence, s_SignalValue);
		s_SignalValue++;

		pComputeCommandList->Begin(nullptr);
		pComputeCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER, &fromGraphicsToComputeBarrier, 1);
		
		VkStridedBufferRegionKHR raygen_shader_sbt_entry{};
		raygen_shader_sbt_entry.buffer = s_pSBT->GetBuffer();
		raygen_shader_sbt_entry.offset = static_cast<VkDeviceSize>(pGraphicsDeviceVk->RayTracingProperties.shaderGroupHandleSize * INDEX_RAYGEN);
		raygen_shader_sbt_entry.size   = pGraphicsDeviceVk->RayTracingProperties.shaderGroupHandleSize;

		VkStridedBufferRegionKHR miss_shader_sbt_entry{};
		miss_shader_sbt_entry.buffer = s_pSBT->GetBuffer();
		miss_shader_sbt_entry.offset = static_cast<VkDeviceSize>(pGraphicsDeviceVk->RayTracingProperties.shaderGroupHandleSize * INDEX_MISS);
		miss_shader_sbt_entry.size   = pGraphicsDeviceVk->RayTracingProperties.shaderGroupHandleSize;

		VkStridedBufferRegionKHR hit_shader_sbt_entry{};
		hit_shader_sbt_entry.buffer = s_pSBT->GetBuffer();
		hit_shader_sbt_entry.offset = static_cast<VkDeviceSize>(pGraphicsDeviceVk->RayTracingProperties.shaderGroupHandleSize * INDEX_CLOSEST_HIT);
		hit_shader_sbt_entry.size   = pGraphicsDeviceVk->RayTracingProperties.shaderGroupHandleSize;

		VkStridedBufferRegionKHR callable_shader_sbt_entry{};

		vkCmdBindPipeline(s_pComputeCommandLists[modFrameIndex]->GetCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, s_Pipeline);
		vkCmdBindDescriptorSets(s_pComputeCommandLists[modFrameIndex]->GetCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, s_PipelineLayout, 0, 1, &s_DescriptorSets[backBufferIndex], 0, 0);

		pGraphicsDeviceVk->vkCmdTraceRaysKHR(
			s_pComputeCommandLists[modFrameIndex]->GetCommandBuffer(),
			&raygen_shader_sbt_entry,
			&miss_shader_sbt_entry,
			&hit_shader_sbt_entry,
			&callable_shader_sbt_entry,
			1920,
			1080,
			1);

		pComputeCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, &fromComputeToGraphicsBarrier, 1);
		pComputeCommandList->End();
		RenderSystem::GetComputeQueue()->ExecuteCommandLists(&pComputeCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, s_pFence, s_SignalValue - 1, s_pFence, s_SignalValue);
		s_SignalValue++;

		pGraphicsPostCommandList->Begin(nullptr);
		pGraphicsPostCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_RAY_TRACING_SHADER, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, &fromComputeToGraphicsBarrier, 1);
		pGraphicsPostCommandList->End();
		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&pGraphicsPostCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, s_pFence, s_SignalValue - 1, s_pFence, s_SignalValue);
		s_SignalValue++;
	}

	void RayTracingTestVK::Debug(IAccelerationStructure* pBLAS, IAccelerationStructure* pTLAS)
	{
		const GraphicsDeviceVK* pGraphicsDeviceVk = reinterpret_cast<const GraphicsDeviceVK*>(RenderSystem::GetDevice());
		AccelerationStructureVK* pBLASVk = reinterpret_cast<AccelerationStructureVK*>(pBLAS);
		AccelerationStructureVK* pTLASVk = reinterpret_cast<AccelerationStructureVK*>(pTLAS);

		VkResult result;

		uint64 serializedTLAS[10];
		uint64 serializedBLAS[10];

		memset(serializedTLAS, 0, sizeof(serializedTLAS));
		memset(serializedBLAS, 0, sizeof(serializedBLAS));

		{
			byte* pMapped;

			byte driverUUID[VK_UUID_SIZE];
			byte compatibility[VK_UUID_SIZE];
			uint64 size;
			uint64 deserializedSize;
			uint64 numASHandles;

			pMapped = reinterpret_cast<byte*>(s_pTLASSerializedBuffer->Map());

			memcpy(&driverUUID,			pMapped,														VK_UUID_SIZE);
			memcpy(&compatibility,		pMapped								+		VK_UUID_SIZE,		VK_UUID_SIZE);
			memcpy(&size,				pMapped								+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(&deserializedSize,	pMapped		+		sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(&numASHandles,		pMapped		+ 2 *	sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(serializedTLAS,		pMapped		+ 3 *	sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		numASHandles * sizeof(uint64));

			LOG_MESSAGE("TLAS Size: %u", size);
			LOG_MESSAGE("TLAS Deserialized Size: %u", deserializedSize);
			LOG_MESSAGE("TLAS Num AS Handles: %u", numASHandles);

			s_pTLASSerializedBuffer->Unmap();
		}

		{
			byte* pMapped;

			byte driverUUID[VK_UUID_SIZE];
			byte compatibility[VK_UUID_SIZE];
			uint64 size;
			uint64 deserializedSize;
			uint64 numASHandles;

			pMapped = reinterpret_cast<byte*>(s_pBLASSerializedBuffer->Map());

			memcpy(&driverUUID,			pMapped,														VK_UUID_SIZE);
			memcpy(&compatibility,		pMapped								+		VK_UUID_SIZE,		VK_UUID_SIZE);
			memcpy(&size,				pMapped								+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(&deserializedSize,	pMapped		+		sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(&numASHandles,		pMapped		+ 2 *	sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		sizeof(uint64));
			memcpy(serializedBLAS,		pMapped		+ 3 *	sizeof(uint64)	+ 2 *	VK_UUID_SIZE,		numASHandles * sizeof(uint64));

			LOG_MESSAGE("BLAS Size: %u", size);
			LOG_MESSAGE("BLAS Deserialized Size: %u", deserializedSize);
			LOG_MESSAGE("BLAS Num AS Handles: %u", numASHandles);

			s_pBLASSerializedBuffer->Unmap();
		}

		int fim = 0;
	}
}