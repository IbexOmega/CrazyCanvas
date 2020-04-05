#include "Log/Log.h"

#include "Rendering/Core/Vulkan/BottomLevelAccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Resources/Mesh.h"

namespace LambdaEngine
{
	BottomLevelAccelerationStructureVK::BottomLevelAccelerationStructureVK(const GraphicsDeviceVK* pDevice) :
		TDeviceChild(pDevice),
		m_AccelerationStructure(VK_NULL_HANDLE),
		m_AccelerationStructureMemory(VK_NULL_HANDLE),
		m_AccelerationStructureBuilt(false)
	{
	}

	BottomLevelAccelerationStructureVK::~BottomLevelAccelerationStructureVK()
	{
		if (m_AccelerationStructure != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_pDevice->Device, m_AccelerationStructureMemory, nullptr);
			m_pDevice->vkDestroyAccelerationStructureKHR(m_pDevice->Device, m_AccelerationStructure, nullptr);
		}
	}

	bool BottomLevelAccelerationStructureVK::Init(const BottomLevelAccelerationStructureDesc& desc)
	{
		m_Desc = desc;

		if (!InitAccelerationStructure(desc))
		{
			return false;
		}

		SetName(desc.pName);

		D_LOG_MESSAGE("[BottomLevelAccelerationStructureVK]: BottomLevelAccelerationStructure \"%s\" initialized!", m_Desc.pName);
		return true;
	}

	void BottomLevelAccelerationStructureVK::UpdateGeometryData(IBuffer* pVertexBuffer, uint32 firstVertexIndex, IBuffer* pIndexBuffer, uint32 indexBufferByteOffset, uint32 triCount, void* pTransform, IBuffer* pScratchBuffer)
	{
		if (!m_AccelerationStructureBuilt)
		{
			//Initial Build
			m_AccelerationStructureBuilt = true;

			VkDeviceOrHostAddressConstKHR vertexDataAddressUnion = {};
			vertexDataAddressUnion.deviceAddress						= pVertexBuffer->GetDeviceAdress();

			VkDeviceOrHostAddressConstKHR indexDataAddressUnion = {};
			indexDataAddressUnion.deviceAddress							= pIndexBuffer->GetDeviceAdress();

			VkDeviceOrHostAddressConstKHR transformDataAddressUnion = {};
			transformDataAddressUnion.hostAddress						= pTransform;

			VkAccelerationStructureGeometryTrianglesDataKHR geometryDataDesc = {};
			geometryDataDesc.sType										= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			geometryDataDesc.vertexFormat								= VK_FORMAT_R32G32B32_SFLOAT;
			geometryDataDesc.vertexData									= vertexDataAddressUnion;
			geometryDataDesc.vertexStride								= sizeof(Vertex);
			geometryDataDesc.indexType									= VK_INDEX_TYPE_UINT32;
			geometryDataDesc.indexData									= indexDataAddressUnion;
			geometryDataDesc.transformData								= transformDataAddressUnion;

			VkAccelerationStructureGeometryDataKHR geometryDataUnion = {};
			geometryDataUnion.triangles									= geometryDataDesc;

			VkAccelerationStructureGeometryKHR geometryData = {};
			geometryData.sType											= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometryData.geometryType									= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			geometryData.geometry										= geometryDataUnion;
			geometryData.flags											= VK_GEOMETRY_OPAQUE_BIT_KHR; //Cant be opaque if we want to utilize any-hit shaders

			VkAccelerationStructureGeometryKHR* pGeometryData = &geometryData;

			VkDeviceOrHostAddressKHR scratchBufferAddressUnion = {};
			scratchBufferAddressUnion.deviceAddress						= pScratchBuffer->GetDeviceAdress();

			VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildInfo = {};
			accelerationStructureBuildInfo.sType						= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			accelerationStructureBuildInfo.type							= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			accelerationStructureBuildInfo.flags						= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			
			if (m_Desc.Updateable)
			{
				accelerationStructureBuildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
			}
			
			accelerationStructureBuildInfo.geometryArrayOfPointers		= VK_FALSE;
			accelerationStructureBuildInfo.geometryCount				= 1;
			accelerationStructureBuildInfo.ppGeometries					= &pGeometryData;
			accelerationStructureBuildInfo.update						= VK_FALSE;
			accelerationStructureBuildInfo.srcAccelerationStructure		= VK_NULL_HANDLE;
			accelerationStructureBuildInfo.dstAccelerationStructure		= m_AccelerationStructure;
			accelerationStructureBuildInfo.scratchData					= scratchBufferAddressUnion;

			VkAccelerationStructureBuildOffsetInfoKHR accelerationStructureOffsetInfo = {};
			accelerationStructureOffsetInfo.primitiveCount				= triCount;
			accelerationStructureOffsetInfo.primitiveOffset				= indexBufferByteOffset;
			accelerationStructureOffsetInfo.firstVertex					= firstVertexIndex;
			accelerationStructureOffsetInfo.transformOffset				= 0;

			VkAccelerationStructureBuildOffsetInfoKHR* pAccelerationStructureOffsetInfo = &accelerationStructureOffsetInfo;

			VkCommandBuffer temp;
			m_pDevice->vkCmdBuildAccelerationStructureKHR(temp, 1, &accelerationStructureBuildInfo, &pAccelerationStructureOffsetInfo);
		}
		else
		{
			if (m_Desc.Updateable)
			{
				LOG_ERROR("[BottomLevelAccelerationStructureVK]: UpdateGeometryData called on built structure, but not implemented");
			}
			else
			{
				LOG_WARNING("[BottomLevelAccelerationStructureVK]: UpdateGeometryData called for structure not updateable \"%s\"!", m_Desc.pName);
			}
		}
	}

	uint64 BottomLevelAccelerationStructureVK::GetScratchMemorySizeRequirement()
	{
		return GetMemoryRequirements(VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR).size;
	}

	void BottomLevelAccelerationStructureVK::SetName(const char* pName)
	{
		m_pDevice->SetVulkanObjectName(pName, (uint64)m_AccelerationStructure, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
	}

	bool BottomLevelAccelerationStructureVK::InitAccelerationStructure(const BottomLevelAccelerationStructureDesc& desc)
	{
		VkAccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo = {};
		geometryTypeInfo.sType											= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		geometryTypeInfo.geometryType									= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometryTypeInfo.maxPrimitiveCount								= desc.MaxTriCount;
		geometryTypeInfo.indexType										= VK_INDEX_TYPE_UINT32;
		geometryTypeInfo.maxVertexCount									= desc.MaxVertCount;
		geometryTypeInfo.vertexFormat									= VK_FORMAT_R32G32B32_SFLOAT;
		geometryTypeInfo.allowsTransforms								= VK_TRUE;

		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
		accelerationStructureCreateInfo.sType							= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.type							= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureCreateInfo.compactedSize					= 0; //Compaction currently not supported'
		accelerationStructureCreateInfo.flags							= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

		if (m_Desc.Updateable)
		{
			accelerationStructureCreateInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
			LOG_WARNING("[BottomLevelAccelerationStructureVK]: Acceleration Structure \"%s\" initialized as Updateable, but not implemented!", m_Desc.pName);
		}

		accelerationStructureCreateInfo.maxGeometryCount				= 1;
		accelerationStructureCreateInfo.pGeometryInfos					= &geometryTypeInfo;
		accelerationStructureCreateInfo.deviceAddress					= VK_NULL_HANDLE;

		if (m_pDevice->vkCreateAccelerationStructureKHR(m_pDevice->Device, &accelerationStructureCreateInfo, nullptr, &m_AccelerationStructure) != VK_SUCCESS)
		{
			LOG_ERROR("[BottomLevelAccelerationStructureVK]: vkCreateAccelerationStructureKHR failed for \"%s\"", m_Desc.pName);
			return false;
		}

		VkMemoryRequirements memoryRequirements							= GetMemoryRequirements(VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR);

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType										= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize								= memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex								= FindMemoryType(m_pDevice->PhysicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(m_pDevice->Device, &memoryAllocateInfo, nullptr, &m_AccelerationStructureMemory) != VK_SUCCESS)
		{
			LOG_ERROR("[BottomLevelAccelerationStructureVK]: vkAllocateMemory failed for \"%s\"", m_Desc.pName);
			return false;
		}

		VkBindAccelerationStructureMemoryInfoKHR accelerationStructureMemoryInfo = {};
		accelerationStructureMemoryInfo.sType							= VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
		accelerationStructureMemoryInfo.accelerationStructure			= m_AccelerationStructure;
		accelerationStructureMemoryInfo.memory							= m_AccelerationStructureMemory;

		if (m_pDevice->vkBindAccelerationStructureMemoryKHR(m_pDevice->Device, 1, &accelerationStructureMemoryInfo) != VK_SUCCESS)
		{
			LOG_ERROR("[BottomLevelAccelerationStructureVK]: vkBindAccelerationStructureMemoryKHR failed for \"%s\"", m_Desc.pName);
			return false;
		}

		VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
		accelerationStructureDeviceAddressInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationStructureDeviceAddressInfo.accelerationStructure	= m_AccelerationStructure;

		m_AccelerationStructureDeviceAddress = m_pDevice->vkGetAccelerationStructureDeviceAddressKHR(m_pDevice->Device, &accelerationStructureDeviceAddressInfo);

		D_LOG_MESSAGE("[BottomLevelAccelerationStructureVK]: Acceleration Structure \"%s\" initialized with size of %u bytes", m_Desc.pName, memoryRequirements.size);
		return false;
	}

	VkMemoryRequirements BottomLevelAccelerationStructureVK::GetMemoryRequirements(VkAccelerationStructureMemoryRequirementsTypeKHR type)
	{
		VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo = {};
		memoryRequirementsInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
		memoryRequirementsInfo.type						= type;
		memoryRequirementsInfo.accelerationStructure	= m_AccelerationStructure;

		VkMemoryRequirements2 memoryRequirements2 = {};
		memoryRequirements2.sType						= VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

		m_pDevice->vkGetAccelerationStructureMemoryRequirementsKHR(m_pDevice->Device, &memoryRequirementsInfo, &memoryRequirements2);

		return memoryRequirements2.memoryRequirements;
	}
}