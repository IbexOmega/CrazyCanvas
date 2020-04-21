#include "Rendering/Core/Vulkan/DeviceAllocatorVK.h"
#include "Rendering/Core/Vulkan/GraphicsDeviceVK.h"
#include "Rendering/Core/Vulkan/VulkanHelpers.h"

namespace LambdaEngine
{
    /*
     * DeviceMemoryBlockVK
     */

    struct DeviceMemoryBlockVK
    {
        DeviceMemoryPageVK*     pPage       = nullptr;
        DeviceMemoryBlockVK*    pNext       = nullptr;
        DeviceMemoryBlockVK*    pPrevious   = nullptr;
        uint64                  SizeInBytes = 0;
        uint64                  Alignment   = 0;
    };

    /*
     * DeviceMemoryPageVK
     */

    class DeviceMemoryPageVK
    {
    public:
        DECL_UNIQUE_CLASS(DeviceMemoryPageVK);
        
        DeviceMemoryPageVK(const GraphicsDeviceVK* pDevice, uint32 memoryIndex)
            : m_pDevice(pDevice),
            m_MemoryIndex(memoryIndex)
        {
        }
        
        ~DeviceMemoryPageVK()
        {
            if (m_MappingCount > 0)
            {
                vkUnmapMemory(m_pDevice->Device, m_DeviceMemory);
                m_DeviceMemory = VK_NULL_HANDLE;
                
                m_pHostMemory   = nullptr;
                m_MappingCount  = 0;
            }
        }
        
        bool Init()
        {
            return true;
        }
        
        bool Allocate(AllocationVK* pAllocation, VkDeviceSize sizeInBytes, VkDeviceSize alignment)
        {
            return true;
        }
        
        bool Free(AllocationVK* pAllocation)
        {
            VALIDATE(pAllocation != nullptr);
            DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
            
            VALIDATE(pBlock != nullptr);
            VALIDATE(ValidateBlock(pBlock));
            
            return true;
        }
        
        void* Map(AllocationVK* pAllocation)
        {
            VALIDATE(pAllocation != nullptr);
            DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
            
            VALIDATE(pBlock != nullptr);
            VALIDATE(ValidateBlock(pBlock));
            
            if (m_MappingCount <= 0)
            {
                vkMapMemory(m_pDevice->Device, m_DeviceMemory, 0, VK_WHOLE_SIZE, 0, (void**)&m_pHostMemory);
            }
            
            VALIDATE(m_pHostMemory != nullptr);
            
            m_MappingCount++;
            return m_pHostMemory + pAllocation->Offset;
        }
        
        void Unmap(AllocationVK* pAllocation)
        {
            VALIDATE(pAllocation != nullptr);
            DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
            
            VALIDATE(pBlock != nullptr);
            VALIDATE(ValidateBlock(pBlock));
            
            m_MappingCount = 0;
            if (m_MappingCount <= 0)
            {
                vkUnmapMemory(m_pDevice->Device, m_DeviceMemory);
            }
        }
        
        FORCEINLINE uint32 GetMemoryIndex() const
        {
            return m_MemoryIndex;
        }
        
    private:
        /*
         * Debug tools
         */
        
        bool ValidateBlock(DeviceMemoryBlockVK* pBlock)
        {
            DeviceMemoryBlockVK* pIterator = m_Head;
            while (pIterator != nullptr)
            {
                pIterator = pIterator->pNext;
                if (pIterator == pBlock)
                {
                    return true;
                }
            }
            
            return false;
        }
        
    private:
        const GraphicsDeviceVK* const   m_pDevice;
        const uint32                    m_MemoryIndex;
        
        DeviceMemoryBlockVK*    m_Head          = nullptr;
        byte*                   m_pHostMemory   = nullptr;
        VkDeviceMemory          m_DeviceMemory  = VK_NULL_HANDLE;
        uint32                  m_MappingCount  = 0;
    };

    /*
     * DeviceAllocatorVK
     */

    DeviceAllocatorVK::DeviceAllocatorVK(const GraphicsDeviceVK* pDevice)
        : TDeviceChild(pDevice),
        m_Statistics(),
        m_Desc()
    {
    }

    DeviceAllocatorVK::~DeviceAllocatorVK()
    {
    }

    bool DeviceAllocatorVK::Init(const DeviceAllocatorDesc* pDesc)
    {
        VALIDATE(pDesc != nullptr);
        return true;
    }

    bool DeviceAllocatorVK::Allocate(AllocationVK* pAllocation, uint64 sizeInBytes, uint64 alignment, uint32 memoryIndex)
    {
        VALIDATE(pAllocation != nullptr);
        
        for (DeviceMemoryPageVK* pMemoryPage : m_Pages)
        {
            if (pMemoryPage->GetMemoryIndex() == memoryIndex)
            {
                
            }
        }
        
        return true;
    }

    bool DeviceAllocatorVK::Free(AllocationVK* pAllocation)
    {
        VALIDATE(pAllocation != nullptr);
        return true;
    }

    void* DeviceAllocatorVK::Map(AllocationVK* pAllocation)
    {
        VALIDATE(pAllocation != nullptr);
        DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
        
        VALIDATE(pBlock != nullptr);
        DeviceMemoryPageVK* pPage = pBlock->pPage;
        
        VALIDATE(pPage != nullptr);
        return pPage->Map(pAllocation);
    }

    void DeviceAllocatorVK::Unmap(AllocationVK* pAllocation)
    {
        VALIDATE(pAllocation != nullptr);
        DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
        
        VALIDATE(pBlock != nullptr);
        DeviceMemoryPageVK* pPage = pBlock->pPage;
        
        VALIDATE(pPage != nullptr);
        return pPage->Unmap(pAllocation);
    }

    void DeviceAllocatorVK::SetName(const char* pName)
    {
    }
}
