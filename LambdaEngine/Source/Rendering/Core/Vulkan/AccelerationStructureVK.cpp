#include "Log/Log.h"

#include "Rendering/Core/Vulkan/AccelerationStructureVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	AccelerationStructureVK::AccelerationStructureVK(const GraphicsDeviceVK* pDevice) 
		: TDeviceChild(pDevice),
		m_Allocation(),
		m_Desc()
	{
	}

	AccelerationStructureVK::~AccelerationStructureVK()
	{
		SAFERELEASE(m_pScratchBuffer);

		if (m_AccelerationStructure != VK_NULL_HANDLE)
		{
			VALIDATE(m_pDevice->vkDestroyAccelerationStructureKHR != nullptr);

			m_pDevice->vkDestroyAccelerationStructureKHR(m_pDevice->Device, m_AccelerationStructure, nullptr);
			m_AccelerationStructure = VK_NULL_HANDLE;

			m_AccelerationStructureDeviceAddress = 0;
		}

		if (m_pAllocator)
		{
			m_pAllocator->Free(&m_Allocation);
			memset(&m_Allocation, 0, sizeof(m_Allocation));

			RELEASE(m_pAllocator);
		}
		else
		{
			if (m_AccelerationStructureMemory != VK_NULL_HANDLE)
			{
				vkFreeMemory(m_pDevice->Device, m_AccelerationStructureMemory, nullptr);
				m_AccelerationStructureMemory = VK_NULL_HANDLE;
			}
		}
	}

	bool AccelerationStructureVK::Init(const AccelerationStructureDesc* pDesc, IDeviceAllocator* pAllocator)
	{
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
		accelerationStructureCreateInfo.sType				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.pNext				= nullptr;
		accelerationStructureCreateInfo.compactedSize		= 0;
		accelerationStructureCreateInfo.maxGeometryCount	= 1;
		accelerationStructureCreateInfo.deviceAddress		= VK_NULL_HANDLE;
		accelerationStructureCreateInfo.flags				= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		if (pDesc->Flags & FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE)
		{
			accelerationStructureCreateInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}

		VkAccelerationStructureCreateGeometryTypeInfoKHR geometryTypeInfo = {};
		geometryTypeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		geometryTypeInfo.pNext = nullptr;
		
		accelerationStructureCreateInfo.pGeometryInfos = &geometryTypeInfo;
		if (pDesc->Type == EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_TOP)
		{
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			
			geometryTypeInfo.geometryType		= VK_GEOMETRY_TYPE_INSTANCES_KHR;
			geometryTypeInfo.maxPrimitiveCount	= pDesc->InstanceCount;
		}
		else
		{
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

			geometryTypeInfo.geometryType		= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			geometryTypeInfo.maxPrimitiveCount	= pDesc->MaxTriangleCount;
			geometryTypeInfo.indexType			= VK_INDEX_TYPE_UINT32;
			geometryTypeInfo.maxVertexCount		= pDesc->MaxVertexCount;
			geometryTypeInfo.vertexFormat		= VK_FORMAT_R32G32B32_SFLOAT;
			geometryTypeInfo.allowsTransforms	= pDesc->AllowsTransform ? VK_TRUE : VK_FALSE;
		}

		VALIDATE(m_pDevice->vkCreateAccelerationStructureKHR != nullptr);

		VkResult result = m_pDevice->vkCreateAccelerationStructureKHR(m_pDevice->Device, &accelerationStructureCreateInfo, nullptr, &m_AccelerationStructure);
		if (result != VK_SUCCESS)
		{
			if (pDesc->pName)
			{
				LOG_VULKAN_ERROR(result, "[AccelerationStructureVK]: vkCreateAccelerationStructureKHR failed for \"%s\"", pDesc->pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[AccelerationStructureVK]: vkCreateAccelerationStructureKHR failed");
			}

			return false;
		}
		else
		{
            memcpy(&m_Desc, pDesc, sizeof(m_Desc));
			SetName(pDesc->pName);
		}

		VkMemoryRequirements	memoryRequirements	= GetMemoryRequirements(VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR);
		int32					memoryTypeIndex		= FindMemoryType(m_pDevice->PhysicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkBindAccelerationStructureMemoryInfoKHR accelerationStructureMemoryInfo = {};
		accelerationStructureMemoryInfo.sType					= VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
		accelerationStructureMemoryInfo.pNext					= nullptr;
		accelerationStructureMemoryInfo.deviceIndexCount		= 0;
		accelerationStructureMemoryInfo.pDeviceIndices			= nullptr;
		accelerationStructureMemoryInfo.accelerationStructure	= m_AccelerationStructure;

		if (pAllocator)
		{
			DeviceAllocatorVK* pAllocatorVk = reinterpret_cast<DeviceAllocatorVK*>(pAllocator);
			if (!pAllocatorVk->Allocate(&m_Allocation, memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex))
			{
				LOG_ERROR("[AccelerationStructureVK]: Failed to allocate memory");
				return false;
			}

			pAllocatorVk->AddRef();
			m_pAllocator = pAllocatorVk;

			accelerationStructureMemoryInfo.memoryOffset	= m_Allocation.Offset;
			accelerationStructureMemoryInfo.memory			= m_Allocation.Memory;
		}
		else
		{
			result = m_pDevice->AllocateMemory(&m_AccelerationStructureMemory, memoryRequirements.size, memoryTypeIndex);
			if (result != VK_SUCCESS)
			{
				LOG_VULKAN_ERROR(result, "[AccelerationStructureVK]: Failed to allocate memory");
				return false;
			}

			accelerationStructureMemoryInfo.memoryOffset	= 0;
			accelerationStructureMemoryInfo.memory			= m_AccelerationStructureMemory;
		}

		VALIDATE(m_pDevice->vkBindAccelerationStructureMemoryKHR != nullptr);

		result = m_pDevice->vkBindAccelerationStructureMemoryKHR(m_pDevice->Device, 1, &accelerationStructureMemoryInfo);
		if (result != VK_SUCCESS)
		{
			if (m_Desc.pName)
			{
				LOG_VULKAN_ERROR(result, "[AccelerationStructureVK]: vkBindAccelerationStructureMemoryKHR failed for \"%s\"", m_Desc.pName);
			}
			else
			{
				LOG_VULKAN_ERROR(result, "[AccelerationStructureVK]: vkBindAccelerationStructureMemoryKHR");
			}

			return false;
		}

		VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
		accelerationStructureDeviceAddressInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationStructureDeviceAddressInfo.pNext					= nullptr;
		accelerationStructureDeviceAddressInfo.accelerationStructure	= m_AccelerationStructure;

		VALIDATE(m_pDevice->vkGetAccelerationStructureDeviceAddressKHR != nullptr);
		m_AccelerationStructureDeviceAddress = m_pDevice->vkGetAccelerationStructureDeviceAddressKHR(m_pDevice->Device, &accelerationStructureDeviceAddressInfo);

		if (m_Desc.pName)
		{
			D_LOG_MESSAGE("[AccelerationStructureVK]: Acceleration Structure \"%s\" created with size of %u bytes", m_Desc.pName, memoryRequirements.size);
		}
		else
		{
			D_LOG_MESSAGE("[AccelerationStructureVK]: Acceleration Structure created with size of %u bytes", memoryRequirements.size);
		}

		VkMemoryRequirements scratchMemoryRequirements = GetMemoryRequirements(VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR);

		BufferDesc scratchBufferDesc = {};
		scratchBufferDesc.pName			= "Acceleration Structure Scratch Buffer";
		scratchBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		scratchBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_RAY_TRACING;
		scratchBufferDesc.SizeInBytes	= scratchMemoryRequirements.size;

		m_pScratchBuffer = reinterpret_cast<BufferVK*>(m_pDevice->CreateBuffer(&scratchBufferDesc, pAllocator));
		return m_pScratchBuffer != nullptr;
	}

	void AccelerationStructureVK::SetName(const char* pName)
	{
		if (pName)
		{
			TDeviceChild::SetName(pName);
			m_pDevice->SetVulkanObjectName(pName, (uint64)m_AccelerationStructure, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);

			m_Desc.pName = m_pDebugName;
		}
	}

	VkMemoryRequirements AccelerationStructureVK::GetMemoryRequirements(VkAccelerationStructureMemoryRequirementsTypeKHR type)
	{
		VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo = {};
		memoryRequirementsInfo.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
		memoryRequirementsInfo.pNext					= nullptr;
		memoryRequirementsInfo.type						= type;
		memoryRequirementsInfo.accelerationStructure	= m_AccelerationStructure;
		memoryRequirementsInfo.buildType				= VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;

		VkMemoryRequirements2 memoryRequirements2 = {};
		memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		memoryRequirements2.pNext = nullptr;

		VALIDATE(m_pDevice->vkGetAccelerationStructureMemoryRequirementsKHR != nullptr);

		m_pDevice->vkGetAccelerationStructureMemoryRequirementsKHR(m_pDevice->Device, &memoryRequirementsInfo, &memoryRequirements2);
		return memoryRequirements2.memoryRequirements;
	}
}
