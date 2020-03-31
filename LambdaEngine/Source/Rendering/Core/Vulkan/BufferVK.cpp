#include "Rendering/Core/Vulkan/BufferVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"

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
            //vkDestroyBuffer();
            m_Buffer = VK_NULL_HANDLE;
        }
        
        if (m_Memory != VK_NULL_HANDLE)
        {
            //Free
            m_Memory = VK_NULL_HANDLE;
        }
    }

    bool BufferVK::Create(const BufferDesc& desc)
    {

        return false;
    }

    void* BufferVK::Map()
    {
        void* pHostMemory = nullptr;
        //vkMapMemory(VkDevice device, m_Memory, 0, 0, 0, &pHostMemory);
        return pHostMemory;
    }

    void BufferVK::Unmap()
    {
        //vkUnmapMemory(VkDevice device, m_Memory);
    }

    void BufferVK::SetName(const char* pName)
    {
        //TODO: Set name
    }
}
