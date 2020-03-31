#include "Rendering/Core/Vulkan/TopLevelAccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/BufferVK.h"


#include "Log/Log.h"

namespace LambdaEngine
{
	TopLevelAccelerationStructureVK::TopLevelAccelerationStructureVK(const GraphicsDeviceVK* pDevice) :
		TDeviceChild(pDevice),
		m_Desc({})
	{
	}

	TopLevelAccelerationStructureVK::~TopLevelAccelerationStructureVK()
	{
	}

	bool TopLevelAccelerationStructureVK::Init(const TopLevelAccelerationStructureDesc& desc)
	{
		m_Desc = desc;

		if (!InitAccelerationStructure())
		{
			return false;
		}

		SetName(desc.pName);
		return false;
	}

	void TopLevelAccelerationStructureVK::UpdateData(IBuffer* pBuffer)
	{
		bool sizeChanged = false;

		BufferVK* pVulkanBuffer = reinterpret_cast<BufferVK*>(pBuffer);

		VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
		bufferDeviceAddressInfo.sType								= VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddressInfo.buffer								= pVulkanBuffer->GetBuffer();

		VkDeviceOrHostAddressConstKHR instancesDataAddressUnion = {};
		instancesDataAddressUnion.deviceAddress						= vkGetBufferDeviceAddress(m_pDevice->Device, &bufferDeviceAddressInfo);

		VkAccelerationStructureGeometryInstancesDataKHR instancesDataDesc = {};
		instancesDataDesc.sType										= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instancesDataDesc.arrayOfPointers							= VK_FALSE;
		instancesDataDesc.data										= instancesDataAddressUnion;

		VkAccelerationStructureGeometryDataKHR geometryDataUnion = {};
		geometryDataUnion.instances									= instancesDataDesc;

		VkAccelerationStructureGeometryKHR geometryData = {};
		geometryData.sType											= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometryData.geometryType									= VK_GEOMETRY_TYPE_INSTANCES_KHR;
		geometryData.geometry										= geometryDataUnion;

		VkAccelerationStructureGeometryKHR* pGeometryData = &geometryData;

		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildInfo = {};
		accelerationStructureBuildInfo.sType						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildInfo.type							= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildInfo.flags						= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		accelerationStructureBuildInfo.geometryArrayOfPointers		= VK_FALSE;
		accelerationStructureBuildInfo.geometryCount				= pBuffer->GetDesc().SizeInBytes / sizeof(VkAccelerationStructureInstanceKHR);
		accelerationStructureBuildInfo.ppGeometries					= &pGeometryData;

		if (sizeChanged)
		{
			//Recreate Structure

			accelerationStructureBuildInfo.update					= VK_FALSE;
			accelerationStructureBuildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
			accelerationStructureBuildInfo.dstAccelerationStructure = m_AccelerationStructure;
		}
		else
		{
			accelerationStructureBuildInfo.update					= VK_TRUE;
			accelerationStructureBuildInfo.srcAccelerationStructure = m_AccelerationStructure;
			accelerationStructureBuildInfo.dstAccelerationStructure = m_AccelerationStructure;
		}

		accelerationStructureBuildInfo.scratchData					= m_ScratchBufferAddressUnion;

		VkAccelerationStructureBuildOffsetInfoKHR accelerationStructureOffsetInfo = {};
		accelerationStructureOffsetInfo.primitiveCount				= pBuffer->GetDesc().SizeInBytes / sizeof(VkAccelerationStructureInstanceKHR);
		accelerationStructureOffsetInfo.primitiveOffset				= 0;

		VkAccelerationStructureBuildOffsetInfoKHR* pAccelerationStructureOffsetInfo = &accelerationStructureOffsetInfo;

		VkCommandBuffer temp;
		m_pDevice->vkCmdBuildAccelerationStructureKHR(temp, 1, &accelerationStructureBuildInfo, &pAccelerationStructureOffsetInfo);
	}

	void TopLevelAccelerationStructureVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_AccelerationStructure, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
	}

	bool TopLevelAccelerationStructureVK::InitAccelerationStructure()
	{
		VkAccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo = {};
		geometryTypeInfo.sType											= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		geometryTypeInfo.geometryType									= VK_GEOMETRY_TYPE_INSTANCES_KHR;

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
		accelerationStructureCreateInfo.sType							= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.type							= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureCreateInfo.compactedSize					= 0; //Compaction currently not supported'
		accelerationStructureCreateInfo.flags							= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		accelerationStructureCreateInfo.maxGeometryCount				= 1;
		accelerationStructureCreateInfo.pGeometryInfos					= &geometryTypeInfo;
		accelerationStructureCreateInfo.deviceAddress					= VK_NULL_HANDLE;

		if (m_pDevice->vkCreateAccelerationStructureKHR(m_pDevice->Device, &accelerationStructureCreateInfo, nullptr, &m_AccelerationStructure) != VK_SUCCESS)
		{
			LOG_ERROR("[TopLevelAccelerationStructure]: vkCreateAccelerationStructureKHR failed for %s", m_Desc.pName);
			return false;
		}

		VkMemoryRequirements memoryRequirements							= GetMemoryRequirements();

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType										= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize								= memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex								= FindMemoryType(m_pDevice->PhysicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(m_pDevice->Device, &memoryAllocateInfo, nullptr, &m_AccelerationStructureMemory) != VK_SUCCESS)
		{
			LOG_ERROR("[TopLevelAccelerationStructure]: vkAllocateMemory failed for %s", m_Desc.pName);
			return false;
		}

		VkBindAccelerationStructureMemoryInfoKHR accelerationStructureMemoryInfo = {};
		accelerationStructureMemoryInfo.sType							= VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
		accelerationStructureMemoryInfo.accelerationStructure			= m_AccelerationStructure;
		accelerationStructureMemoryInfo.memory							= m_AccelerationStructureMemory;

		if (m_pDevice->vkBindAccelerationStructureMemoryKHR(m_pDevice->Device, 1, &accelerationStructureMemoryInfo) != VK_SUCCESS)
		{
			LOG_ERROR("[TopLevelAccelerationStructure]: vkBindAccelerationStructureMemoryKHR failed for %s", m_Desc.pName);
			return false;
		}

		VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
		accelerationStructureDeviceAddressInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationStructureDeviceAddressInfo.accelerationStructure	= m_AccelerationStructure;

		m_AccelerationStructureDeviceAddress = m_pDevice->vkGetAccelerationStructureDeviceAddressKHR(m_pDevice->Device, &accelerationStructureDeviceAddressInfo);

		return true;
	}

	bool TopLevelAccelerationStructureVK::InitScratchBuffer()
	{
		return true;
	}

	void TopLevelAccelerationStructureVK::UpdateScratchBuffer()
	{
		VkMemoryRequirements memoryRequirements = GetMemoryRequirements();

		if (m_pScratchBuffer->GetDesc().SizeInBytes < memoryRequirements.size)
		{
			SAFEDELETE(m_pScratchBuffer);

			BufferDesc scratchBufferDesc = {};
			scratchBufferDesc.pName			= "TLAS Scratch Buffer";
			scratchBufferDesc.MemoryType	= EMemoryType::GPU_MEMORY;
			scratchBufferDesc.Flags			= EBufferFlags::BUFFER_FLAG_RAY_TRACING;
			scratchBufferDesc.SizeInBytes	= memoryRequirements.size;

			m_pScratchBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(scratchBufferDesc));

			VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
			bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			bufferDeviceAddressInfo.buffer = m_pScratchBuffer->GetBuffer();

			m_ScratchBufferAddressUnion.deviceAddress = vkGetBufferDeviceAddress(m_pDevice->Device, &bufferDeviceAddressInfo);

			D_LOG_MESSAGE("Reallocated Scratch Buffer for TLAS %s, new size %u bytes!", m_Desc.pName, memoryRequirements.size);
		}
	}

	VkMemoryRequirements TopLevelAccelerationStructureVK::GetMemoryRequirements()
	{
		VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo = {};
		memoryRequirementsInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
		memoryRequirementsInfo.type						= VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV;
		memoryRequirementsInfo.accelerationStructure	= m_AccelerationStructure;

		VkMemoryRequirements2 memoryRequirements2 = {};
		memoryRequirements2.sType						= VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		m_pDevice->vkGetAccelerationStructureMemoryRequirementsKHR(m_pDevice->Device, &memoryRequirementsInfo, &memoryRequirements2);

		return memoryRequirements2.memoryRequirements;
	}
}