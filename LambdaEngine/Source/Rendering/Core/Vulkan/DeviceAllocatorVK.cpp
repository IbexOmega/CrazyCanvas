#include "Log/Log.h"

#include "Math/MathUtilities.h"

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
        DeviceMemoryPageVK*     pPage               = nullptr;
        DeviceMemoryBlockVK*    pNext               = nullptr;
        DeviceMemoryBlockVK*    pPrevious           = nullptr;
        
        // Size of the allocation
        uint64 SizeInBytes = 0;
        
        // Totoal size of the block (TotalSizeInBytes - SizeInBytes = AlignmentOffset)
        uint64 TotalSizeInBytes = 0;
        
        // Offset of the DeviceMemory
        uint64  Offset = 0;
        bool    IsFree = true;
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
        
        bool Init(uint64 sizeInBytes)
        {
            VkResult result = m_pDevice->AllocateMemory(&m_DeviceMemory, sizeInBytes, m_MemoryIndex);
            if (result != VK_SUCCESS)
            {
                LOG_VULKAN_ERROR(result, "[DeviceMemoryPageVK]: Failed to allocate memory");
                return false;
            }
            else
            {
                m_pHead = DBG_NEW DeviceMemoryBlockVK();
                m_pHead->pPage              = this;
                m_pHead->pNext              = nullptr;
                m_pHead->pPrevious          = nullptr;
                m_pHead->TotalSizeInBytes   = sizeInBytes;
                m_pHead->SizeInBytes        = sizeInBytes;
                m_pHead->Offset             = 0;
                m_pHead->IsFree             = true;
                
                return true;
            }
        }
        
        bool Allocate(AllocationVK* pAllocation, VkDeviceSize sizeInBytes, VkDeviceSize alignment, VkDeviceSize pageGranularity)
        {
            VALIDATE(pAllocation != nullptr);
            
            uint64 paddedOffset = 0;
            uint64 padding      = 0;
            
            // Find a suitable block
            DeviceMemoryBlockVK* pBestFit = nullptr;
            for (DeviceMemoryBlockVK* pIterator = m_pHead; pIterator != nullptr; pIterator = pIterator->pNext)
            {
                if (!pIterator->IsFree)
                {
                    continue;
                }
                
                if (pIterator->SizeInBytes < sizeInBytes)
                {
                    continue;
                }
                
                uint64 paddedOffset = AlignUp(pIterator->Offset, alignment);
                if (pageGranularity > 1)
                {
                    DeviceMemoryBlockVK* pNext      = pIterator->pNext;
                    DeviceMemoryBlockVK* pPrevious  = pIterator->pPrevious;
                    
                    if (pPrevious)
                    {
                        if (IsAliasing(pPrevious->Offset, pPrevious->TotalSizeInBytes, paddedOffset, pageGranularity))
                        {
                            paddedOffset = AlignUp(paddedOffset, pageGranularity);
                        }
                    }
                    
                    if (pNext)
                    {
                        if (IsAliasing(paddedOffset, sizeInBytes, pNext->Offset, pageGranularity))
                        {
                            continue;
                        }
                    }
                }
                
                padding = paddedOffset - pIterator->Offset;
                if (pIterator->SizeInBytes >= (sizeInBytes + padding))
                {
                    pBestFit = pIterator;
                    break;
                }
            }
            
            VALIDATE(pBestFit != nullptr);
            
            // Divide block
            const uint64 paddedSizeInBytes = (padding + sizeInBytes);
            if (pBestFit->SizeInBytes > paddedSizeInBytes)
            {
                DeviceMemoryBlockVK* pNewBlock = DBG_NEW DeviceMemoryBlockVK();
                pNewBlock->Offset           = pBestFit->Offset + paddedSizeInBytes;
                pNewBlock->pNext            = pBestFit->pNext;
                pNewBlock->pPrevious        = pBestFit;
                pNewBlock->SizeInBytes      = pBestFit->SizeInBytes - paddedSizeInBytes;
                pNewBlock->TotalSizeInBytes = pNewBlock->SizeInBytes;
                pNewBlock->IsFree           = true;
                
                pBestFit->pNext = pNewBlock;
            }
            
            // Set new attributes of block
            pBestFit->SizeInBytes       = sizeInBytes;
            pBestFit->TotalSizeInBytes  = paddedSizeInBytes;
            pBestFit->IsFree            = false;
            
            // Setup allocation
            pAllocation->Memory = m_DeviceMemory;
            pAllocation->Offset = paddedOffset;
            pAllocation->pBlock = pBestFit;
            
            VALIDATE(ValidateNoOverlap());
            return true;
        }
        
        bool Free(AllocationVK* pAllocation)
        {
            VALIDATE(pAllocation != nullptr);
            DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
            
            VALIDATE(pBlock != nullptr);
            VALIDATE(ValidateBlock(pBlock));
            
                        
            
            VALIDATE(ValidateNoOverlap());
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
        bool IsAliasing(VkDeviceSize aOffset, VkDeviceSize aSize, VkDeviceSize bOffset, VkDeviceSize pageGranularity)
        {
            VALIDATE((aOffset + aSize) <= bOffset);
            VALIDATE(aSize > 0);
            VALIDATE(pageGranularity > 0);
            
            VkDeviceSize aEnd       = aOffset + (aSize - 1);
            VkDeviceSize aEndPage   = aEnd & ~(pageGranularity - 1);
            VkDeviceSize bStart     = bOffset;
            VkDeviceSize bStartPage = bStart & ~(pageGranularity - 1);
            return aEndPage >= bStartPage;
        }
        
        /*
         * Debug tools
         */
        
        bool ValidateBlock(DeviceMemoryBlockVK* pBlock)
        {
            DeviceMemoryBlockVK* pIterator = m_pHead;
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
        
        bool ValidateNoOverlap()
        {
            DeviceMemoryBlockVK* pIterator = m_pHead;
            while (pIterator != nullptr)
            {
                if (pIterator->pNext)
                {
                    if ((pIterator->Offset + pIterator->TotalSizeInBytes) > (pIterator->pNext->Offset))
                    {
                        D_LOG_WARNING("[DeviceMemoryPageVK]: Overlap found");
                        return false;
                    }
                }
                
                pIterator = pIterator->pNext;
            }
            
            return true;
        }
        
    private:
        const GraphicsDeviceVK* const   m_pDevice;
        const uint32                    m_MemoryIndex;
        
        DeviceMemoryBlockVK*    m_pHead         = nullptr;
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
        
        memcpy(&m_Desc, pDesc, sizeof(m_Desc));
        if (pDesc->pName)
        {
            SetName(pDesc->pName);
        }
        
        m_DeviceProperties = m_pDevice->GetPhysicalDeviceProperties();
        
        return true;
    }

    bool DeviceAllocatorVK::Allocate(AllocationVK* pAllocation, uint64 sizeInBytes, uint64 alignment, uint32 memoryIndex)
    {
        VALIDATE(pAllocation != nullptr);
        
        if (!m_Pages.empty())
        {
            for (DeviceMemoryPageVK* pMemoryPage : m_Pages)
            {
                VALIDATE(pMemoryPage != nullptr);
                if (pMemoryPage->GetMemoryIndex() == memoryIndex)
                {
                    return pMemoryPage->Allocate(pAllocation, sizeInBytes, alignment, m_DeviceProperties.limits.bufferImageGranularity);
                }
            }
        }
        
        DeviceMemoryPageVK* pNewMemoryPage = DBG_NEW DeviceMemoryPageVK(m_pDevice, memoryIndex);
        if (!pNewMemoryPage->Init(m_Desc.PageSizeInBytes))
        {
            return false;
        }
        
        m_Pages.emplace_back(pNewMemoryPage);
        return pNewMemoryPage->Allocate(pAllocation, sizeInBytes, alignment, m_DeviceProperties.limits.bufferImageGranularity);
    }

    bool DeviceAllocatorVK::Free(AllocationVK* pAllocation)
    {
        VALIDATE(pAllocation != nullptr);
        DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
        
        VALIDATE(pBlock != nullptr);
        DeviceMemoryPageVK* pPage = pBlock->pPage;
        
        VALIDATE(pPage != nullptr);
        return pPage->Free(pAllocation);
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
        //TODO: Create this function
    }
}
