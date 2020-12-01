#include "Log/Log.h"

#include <algorithm>

#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
	BufferVK::BufferVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
		, m_Allocation()
	{
	}

	BufferVK::~BufferVK()
	{
		if (m_IsMapped)
		{
			Unmap();
		}

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(m_pDevice->Device, m_Buffer, nullptr);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_Allocation.Memory != VK_NULL_HANDLE)
		{
			m_pDevice->FreeMemory(&m_Allocation);
		}

		ZERO_MEMORY(&m_Allocation, sizeof(m_Allocation));
	}

	bool BufferVK::Init(const BufferDesc* pDesc)
	{
		VkBufferCreateInfo info = {};
		info.sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.pNext                  = nullptr;
		info.flags                  = 0;
		info.pQueueFamilyIndices    = nullptr;
		info.queueFamilyIndexCount  = 0;
		info.sharingMode            = VK_SHARING_MODE_EXCLUSIVE;
		info.size                   = pDesc->SizeInBytes;

		VALIDATE(info.size > 0);

		VkPhysicalDeviceProperties deviceProperties = m_pDevice->GetPhysicalDeviceProperties();
		VkPhysicalDeviceLimits& deviceLimits = deviceProperties.limits;
		m_AlignmentRequirement = 1LLU;

		info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		if (pDesc->Flags & FBufferFlag::BUFFER_FLAG_VERTEX_BUFFER)
		{
			info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			m_AlignmentRequirement = std::max(m_AlignmentRequirement, 1LLU);
		}
		if (pDesc->Flags & FBufferFlag::BUFFER_FLAG_INDEX_BUFFER)
		{
			info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			m_AlignmentRequirement = std::max(m_AlignmentRequirement, 1LLU);
		}
		if (pDesc->Flags & FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER)
		{
			info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			m_AlignmentRequirement = std::max(m_AlignmentRequirement, deviceLimits.minUniformBufferOffsetAlignment);
		}
		if (pDesc->Flags & FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER)
		{
			info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			m_AlignmentRequirement = std::max(m_AlignmentRequirement, deviceLimits.minStorageBufferOffsetAlignment);
		}
		if (pDesc->Flags & FBufferFlag::BUFFER_FLAG_COPY_DST)
		{
			info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			m_AlignmentRequirement = std::max(m_AlignmentRequirement, 1LLU);
		}
		if (pDesc->Flags & FBufferFlag::BUFFER_FLAG_COPY_SRC)
		{
			info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			m_AlignmentRequirement = std::max(m_AlignmentRequirement, 1LLU);
		}
		if (pDesc->Flags & FBufferFlag::BUFFER_FLAG_RAY_TRACING)
		{
			info.usage |= VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR;
			m_AlignmentRequirement = std::max(m_AlignmentRequirement, 1LLU);
		}
		if (pDesc->Flags & FBufferFlag::BUFFER_FLAG_INDIRECT_BUFFER)
		{
			info.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			m_AlignmentRequirement = std::max(m_AlignmentRequirement, 1LLU);
		}

		VkResult result = vkCreateBuffer(m_pDevice->Device, &info, nullptr, &m_Buffer);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "Failed to create buffer");
			return false;
		}
		else
		{
			LOG_DEBUG("Created Buffer");

			m_Desc = *pDesc;
			SetName(m_Desc.DebugName);
		}

		VkMemoryRequirements memoryRequirements = { };
		vkGetBufferMemoryRequirements(m_pDevice->Device, m_Buffer, &memoryRequirements);

		VkMemoryPropertyFlags memoryProperties = 0;
		if (m_Desc.MemoryType == EMemoryType::MEMORY_TYPE_CPU_VISIBLE)
		{
			memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}
		else if (m_Desc.MemoryType == EMemoryType::MEMORY_TYPE_GPU)
		{
			memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}

		int32 memoryTypeIndex = FindMemoryType(m_pDevice->PhysicalDevice, memoryRequirements.memoryTypeBits, memoryProperties);
		if (!m_pDevice->AllocateBufferMemory(&m_Allocation, m_Desc.Flags, memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex))
		{
			LOG_ERROR("Failed to allocate memory");
			return false;
		}

		result = vkBindBufferMemory(m_pDevice->Device, m_Buffer, m_Allocation.Memory, m_Allocation.Offset);
		if (result != VK_SUCCESS)
		{
			LOG_VULKAN_ERROR(result, "[BufferVK]: Failed to bind memory");
			return false;
		}

		VkBufferDeviceAddressInfo deviceAdressInfo = { };
		deviceAdressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		deviceAdressInfo.pNext  = nullptr;
		deviceAdressInfo.buffer = m_Buffer;

		m_DeviceAddress = vkGetBufferDeviceAddress(m_pDevice->Device, &deviceAdressInfo);

		return true;
	}

	void* BufferVK::Map()
	{
		VALIDATE(!m_IsMapped);

		m_IsMapped = true;
		return m_pDevice->MapBufferMemory(&m_Allocation);
	}

	void BufferVK::Unmap()
	{
		VALIDATE(m_IsMapped);

		m_IsMapped = false;
		m_pDevice->UnmapBufferMemory(&m_Allocation);
	}

	void BufferVK::SetName(const String& debugname)
	{
		m_pDevice->SetVulkanObjectName(debugname, reinterpret_cast<uint64>(m_Buffer), VK_OBJECT_TYPE_BUFFER);
		m_Desc.DebugName = debugname;
	}

	uint64 BufferVK::GetDeviceAddress() const
	{
		return static_cast<uint64>(m_DeviceAddress);
	}

	uint64 BufferVK::GetAlignmentRequirement() const
	{
		return m_AlignmentRequirement;
	}
}
