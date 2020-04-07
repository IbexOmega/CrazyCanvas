#include "Log/Log.h"

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
        if (m_Buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_pDevice->Device, m_Buffer, nullptr);
            m_Buffer = VK_NULL_HANDLE;
        }
        
        if (m_Memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_pDevice->Device, m_Memory, nullptr);
            m_Memory = VK_NULL_HANDLE;
        }
    }

    bool BufferVK::Init(const BufferDesc& desc)
    {
        VkBufferCreateInfo info = {};
        info.sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.pNext                  = nullptr;
        info.flags                  = 0;
        info.pQueueFamilyIndices    = nullptr;
        info.queueFamilyIndexCount  = 0;
        info.sharingMode            = VK_SHARING_MODE_EXCLUSIVE;
        info.size                   = desc.SizeInBytes;
        
		info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        if (desc.Flags & EBufferFlags::BUFFER_FLAG_VERTEX_BUFFER)
        {
            info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (desc.Flags & EBufferFlags::BUFFER_FLAG_INDEX_BUFFER)
        {
            info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (desc.Flags & EBufferFlags::BUFFER_FLAG_CONSTANT_BUFFER)
        {
            info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if (desc.Flags & EBufferFlags::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER)
        {
            info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (desc.Flags & EBufferFlags::BUFFER_FLAG_COPY_DST)
        {
            info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        if (desc.Flags & EBufferFlags::BUFFER_FLAG_COPY_SRC)
        {
            info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
		if (desc.Flags & EBufferFlags::BUFFER_FLAG_RAY_TRACING)
		{
			info.usage |= VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR;
		}
		
        VkResult result = vkCreateBuffer(m_pDevice->Device, &info, nullptr, &m_Buffer);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR("[BufferVK]: Failed to create buffer", result);
            return false;
        }
        else
        {
            D_LOG_MESSAGE("[BufferVK]: Created Buffer");

            this->SetName(desc.pName);
            m_Desc = desc;
        }

        //TODO: Allocate with DeviceAllocator
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

		VkMemoryAllocateFlagsInfo allocateFlagsInfo = {};
		allocateFlagsInfo.sType			= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		allocateFlagsInfo.flags			= VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        VkMemoryAllocateInfo allocateInfo = { };
        allocateInfo.sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext              = &allocateFlagsInfo;
        allocateInfo.memoryTypeIndex    = memoryTypeIndex;
        allocateInfo.allocationSize     = memoryRequirements.size;

        result = vkAllocateMemory(m_pDevice->Device, &allocateInfo, nullptr, &m_Memory);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR("[BufferVK]: Failed to allocate memory", result);
            return false;
        }
        else
        {
            D_LOG_MESSAGE("[BufferVK]: Allocated %d bytes to buffer \"%s\"", memoryRequirements.size, desc.pName);
        }

        result = vkBindBufferMemory(m_pDevice->Device, m_Buffer, m_Memory, 0);
        if (result != VK_SUCCESS)
        {
            LOG_VULKAN_ERROR("[BufferVK]: Failed to bind memory.", result);
            return false;
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
        if (vkMapMemory(m_pDevice->Device, m_Memory, 0, VK_WHOLE_SIZE, 0, &pHostMemory) != VK_SUCCESS)
        {
            return nullptr;
        }

        return pHostMemory;
    }

    void BufferVK::Unmap()
    {
        vkUnmapMemory(m_pDevice->Device, m_Memory);
    }

    void BufferVK::SetName(const char* pName)
    {
        m_pDevice->SetVulkanObjectName(pName, (uint64)m_Buffer, VK_OBJECT_TYPE_BUFFER);
    }
    
    uint64 BufferVK::GetDeviceAdress() const
    {
        return uint64(m_DeviceAddress);
    }
}
