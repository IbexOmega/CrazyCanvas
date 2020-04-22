#include "Log/Log.h"

#include <algorithm>

#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
    BufferVK::BufferVK(const GraphicsDeviceVK* pDevice)
        : TDeviceChild(pDevice),
        m_Desc()
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
        
        if (m_pAllocator)
        {
            m_pAllocator->Free(&m_Allocation);
            memset(&m_Allocation, 0, sizeof(m_Allocation));
            
            RELEASE(m_pAllocator);
        }
        else
        {
            if (m_Memory != VK_NULL_HANDLE)
            {
                vkFreeMemory(m_pDevice->Device, m_Memory, nullptr);
                m_Memory = VK_NULL_HANDLE;
            }
        }
    }

    bool BufferVK::Init(const BufferDesc* pDesc, IDeviceAllocator* pAllocator)
    {
        VkBufferCreateInfo info = {};
        info.sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.pNext                  = nullptr;
        info.flags                  = 0;
        info.pQueueFamilyIndices    = nullptr;
        info.queueFamilyIndexCount  = 0;
        info.sharingMode            = VK_SHARING_MODE_EXCLUSIVE;
        info.size                   = pDesc->SizeInBytes;
        
        VkPhysicalDeviceProperties  deviceProperties    = m_pDevice->GetPhysicalDeviceProperties();
        VkPhysicalDeviceLimits&     deviceLimits        = deviceProperties.limits;
        m_AlignementRequirement = 1LLU;

		info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if (pDesc->Flags & FBufferFlags::BUFFER_FLAG_VERTEX_BUFFER)
        {
            info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            m_AlignementRequirement = std::max(m_AlignementRequirement, 1LLU);
        }
        if (pDesc->Flags & FBufferFlags::BUFFER_FLAG_INDEX_BUFFER)
        {
            info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            m_AlignementRequirement = std::max(m_AlignementRequirement, 1LLU);
        }
        if (pDesc->Flags & FBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER)
        {
            info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            m_AlignementRequirement = std::max(m_AlignementRequirement, deviceLimits.minUniformBufferOffsetAlignment);
        }
        if (pDesc->Flags & FBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER)
        {
            info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            m_AlignementRequirement = std::max(m_AlignementRequirement, deviceLimits.minStorageBufferOffsetAlignment);
        }
        if (pDesc->Flags & FBufferFlags::BUFFER_FLAG_COPY_DST)
        {
            info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            m_AlignementRequirement = std::max(m_AlignementRequirement, 1LLU);
        }
        if (pDesc->Flags & FBufferFlags::BUFFER_FLAG_COPY_SRC)
        {
            info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            m_AlignementRequirement = std::max(m_AlignementRequirement, 1LLU);
        }
		if (pDesc->Flags & FBufferFlags::BUFFER_FLAG_RAY_TRACING)
		{
			info.usage |= VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR;
            m_AlignementRequirement = std::max(m_AlignementRequirement, 1LLU);
		}
		if (pDesc->Flags & FBufferFlags::BUFFER_FLAG_INDIRECT_BUFFER)
		{
			info.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			m_AlignementRequirement = std::max(m_AlignementRequirement, 1LLU);
		}
		
        VkResult result = vkCreateBuffer(m_pDevice->Device, &info, nullptr, &m_Buffer);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR(result, "[BufferVK]: Failed to create buffer");
            return false;
        }
        else
        {
            D_LOG_MESSAGE("[BufferVK]: Created Buffer");

            memcpy(&m_Desc, pDesc, sizeof(m_Desc));
            SetName(pDesc->pName);
        }

        VkMemoryRequirements memoryRequirements = { };
        vkGetBufferMemoryRequirements(m_pDevice->Device, m_Buffer, &memoryRequirements);

        VkMemoryPropertyFlags memoryProperties = 0;
        if (m_Desc.MemoryType == EMemoryType::MEMORY_CPU_VISIBLE)
        {
            memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }
        else if (m_Desc.MemoryType == EMemoryType::MEMORY_GPU)
        {
            memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        int32 memoryTypeIndex = FindMemoryType(m_pDevice->PhysicalDevice, memoryRequirements.memoryTypeBits, memoryProperties);
        if (pAllocator)
        {
            DeviceAllocatorVK* pAllocatorVk = reinterpret_cast<DeviceAllocatorVK*>(pAllocator);
            if (!pAllocatorVk->Allocate(&m_Allocation, memoryRequirements.size, memoryRequirements.alignment, memoryTypeIndex))
            {
                LOG_ERROR("[BufferVK]: Failed to allocate memory");
                return false;
            }

            pAllocatorVk->AddRef();
            m_pAllocator = pAllocatorVk;
            
            result = vkBindBufferMemory(m_pDevice->Device, m_Buffer, m_Allocation.Memory, m_Allocation.Offset);
            if (result != VK_SUCCESS)
            {
                LOG_VULKAN_ERROR(result, "[BufferVK]: Failed to bind memory");
                return false;
            }
        }
        else
        {
            result = m_pDevice->AllocateMemory(&m_Memory, memoryRequirements.size, memoryTypeIndex);
            if (result != VK_SUCCESS)
            {
                LOG_VULKAN_ERROR(result, "[BufferVK]: Failed to allocate memory");
                return false;
            }
            
            result = vkBindBufferMemory(m_pDevice->Device, m_Buffer, m_Memory, 0);
            if (result != VK_SUCCESS)
            {
                LOG_VULKAN_ERROR(result, "[BufferVK]: Failed to bind memory");
                return false;
            }
        }

        VkBufferDeviceAddressInfo deviceAdressInfo = { };
        deviceAdressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAdressInfo.pNext  = nullptr;
        deviceAdressInfo.buffer = m_Buffer;

        if (m_pDevice->vkGetBufferDeviceAddress)
        {
            m_DeviceAddress = m_pDevice->vkGetBufferDeviceAddress(m_pDevice->Device, &deviceAdressInfo);
        }

        return true;
    }

    void* BufferVK::Map()
    {
        void* pHostMemory = nullptr;
        if (m_pAllocator)
        {
            pHostMemory = m_pAllocator->Map(&m_Allocation);
            m_IsMapped = true;
        }
        else
        {
		    VkResult result = vkMapMemory(m_pDevice->Device, m_Memory, 0, VK_WHOLE_SIZE, 0, &pHostMemory);
            if (result != VK_SUCCESS)
            {
                if (m_pDebugName)
                {
                    LOG_VULKAN_ERROR(result, "[BufferVK]: Failed to map buffer %s", m_pDebugName);
                }
                else
                {
                    LOG_VULKAN_ERROR(result, "[BufferVK]: Failed to map buffer");
                }
            
                return nullptr;
            }
            else
            {
                m_IsMapped = true;
            }
        }

        return pHostMemory;
    }

    void BufferVK::Unmap()
    {
        ASSERT(m_IsMapped);
        
        if (m_pAllocator)
        {
            m_pAllocator->Unmap(&m_Allocation);
        }
        else
        {
            vkUnmapMemory(m_pDevice->Device, m_Memory);
        }

        m_IsMapped = false;
    }

    void BufferVK::SetName(const char* pName)
    {
        if (pName)
        {
            TDeviceChild::SetName(pName);
            m_pDevice->SetVulkanObjectName(pName, (uint64)m_Buffer, VK_OBJECT_TYPE_BUFFER);

            m_Desc.pName = m_pDebugName;
        }
    }
    
    uint64 BufferVK::GetDeviceAdress() const
    {
        return uint64(m_DeviceAddress);
    }
	
    uint64 BufferVK::GetAlignmentRequirement() const
	{
		return m_AlignementRequirement;
	}
}
