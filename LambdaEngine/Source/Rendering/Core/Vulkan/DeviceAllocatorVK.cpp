#include "Log/Log.h"

#include <mutex>
#include <string>

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
		DeviceMemoryPageVK* pPage = nullptr;

		// Linked list of blocks
		DeviceMemoryBlockVK* pNext		= nullptr;
		DeviceMemoryBlockVK* pPrevious	= nullptr;
		
		// Size of the allocation
		uint64 SizeInBytes = 0;
		
		// Totoal size of the block (TotalSizeInBytes - SizeInBytes = AlignmentOffset)
		uint64 TotalSizeInBytes = 0;
		
		// Offset of the DeviceMemory
		uint64	Offset = 0;
		bool	IsFree = true;
	};

	/*
	 * DeviceMemoryPageVK
	 */
	class DeviceMemoryPageVK
	{
	public:
		DECL_UNIQUE_CLASS(DeviceMemoryPageVK);
		
		DeviceMemoryPageVK(const GraphicsDeviceVK* pDevice, DeviceAllocatorVK* pOwner, const uint32 id, const uint32 memoryIndex)
			: m_pDevice(pDevice)
			, m_pOwningAllocator(pOwner)
			, m_MemoryIndex(memoryIndex)
			, m_ID(id)
		{
		}
		
		~DeviceMemoryPageVK()
		{
			VALIDATE(ValidateBlock(m_pHead));
			VALIDATE(ValidateNoOverlap());

#ifdef LAMBDA_DEVELOPMENT
			if (m_pHead->pNext != nullptr)
			{
				LOG_WARNING("[DeviceMemoryPageVK]: Memoryleak detected, m_pHead->pNext is not nullptr");
			}

			if (m_pHead->pPrevious != nullptr)
			{
				LOG_WARNING("[DeviceMemoryPageVK]: Memoryleak detected, m_pHead->pPrevious is not nullptr");
			}
#endif
			DeviceMemoryBlockVK* pIterator = m_pHead;
			while (pIterator != nullptr)
			{
				DeviceMemoryBlockVK* pBlock = pIterator;
				pIterator = pBlock->pNext;

				SAFEDELETE(pBlock);
			}

			if (m_MappingCount > 0)
			{
				vkUnmapMemory(m_pDevice->Device, m_DeviceMemory);
				m_DeviceMemory = VK_NULL_HANDLE;
				
				m_pHostMemory   = nullptr;
				m_MappingCount  = 0;
			}

			vkFreeMemory(m_pDevice->Device, m_DeviceMemory, nullptr);
			m_DeviceMemory = VK_NULL_HANDLE;
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
				m_pHead->TotalSizeInBytes   = sizeInBytes;
				m_pHead->SizeInBytes        = sizeInBytes;
				
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
			
			VALIDATE(ValidateBlock(m_pHead));

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
				
				paddedOffset = AlignUp(pIterator->Offset, alignment);
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
			
			if (pBestFit == nullptr)
			{
				return false;
			}
			
			// Divide block
			const uint64 paddedSizeInBytes = (padding + sizeInBytes);
			if (pBestFit->SizeInBytes > paddedSizeInBytes)
			{
				DeviceMemoryBlockVK* pNewBlock = DBG_NEW DeviceMemoryBlockVK();
				pNewBlock->Offset           = pBestFit->Offset + paddedSizeInBytes;
				pNewBlock->pPage            = this;
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
			pAllocation->pAllocator = m_pOwningAllocator;

			VALIDATE(ValidateNoOverlap());
			return true;
		}
		
		bool Free(AllocationVK* pAllocation)
		{
			VALIDATE(pAllocation != nullptr);

			DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
			pBlock->IsFree = true;

			VALIDATE(pBlock != nullptr);
			VALIDATE(ValidateBlock(pBlock));
			
			DeviceMemoryBlockVK* pPrevious = pBlock->pPrevious;
			if (pPrevious)
			{
				if (pPrevious->IsFree)
				{
					pPrevious->pNext = pBlock->pNext;
					if (pBlock->pNext)
					{
						pBlock->pNext->pPrevious = pPrevious;
					}

					pPrevious->SizeInBytes      += pBlock->TotalSizeInBytes;
					pPrevious->TotalSizeInBytes += pBlock->TotalSizeInBytes;

					SAFEDELETE(pBlock);
					pBlock = pPrevious;
				}
			}

			DeviceMemoryBlockVK* pNext = pBlock->pNext;
			if (pNext)
			{
				if (pNext->IsFree)
				{
					if (pNext->pNext)
					{
						pNext->pNext->pPrevious = pBlock;
					}
					pBlock->pNext = pNext->pNext;

					pBlock->SizeInBytes         += pNext->TotalSizeInBytes;
					pBlock->TotalSizeInBytes    += pNext->TotalSizeInBytes;

					SAFEDELETE(pNext);
				}
			}
			
			VALIDATE(ValidateNoOverlap());

			pAllocation->Memory = VK_NULL_HANDLE;
			pAllocation->Offset = 0;
			pAllocation->pBlock = nullptr;

			return true;
		}
		
		void* Map(const AllocationVK* pAllocation)
		{
			VALIDATE(pAllocation            != nullptr);
			VALIDATE(pAllocation->pBlock    != nullptr);
			VALIDATE(ValidateBlock(pAllocation->pBlock));
			
			if (m_MappingCount <= 0)
			{
				vkMapMemory(m_pDevice->Device, m_DeviceMemory, 0, VK_WHOLE_SIZE, 0, (void**)&m_pHostMemory);
			}
			
			VALIDATE(m_pHostMemory != nullptr);
			
			m_MappingCount++;
			return m_pHostMemory + pAllocation->Offset;
		}
		
		void Unmap(const AllocationVK* pAllocation)
		{
			VALIDATE(pAllocation != nullptr);
			VALIDATE(pAllocation->pBlock != nullptr);
			VALIDATE(ValidateBlock(pAllocation->pBlock));

			UNREFERENCED_VARIABLE(pAllocation);
			
			m_MappingCount--;
			if (m_MappingCount <= 0)
			{
				vkUnmapMemory(m_pDevice->Device, m_DeviceMemory);
				m_MappingCount = 0;
			}
		}

		void SetName(const String& debugName)
		{
			m_pDevice->SetVulkanObjectName(debugName, reinterpret_cast<uint64>(m_DeviceMemory), VK_OBJECT_TYPE_DEVICE_MEMORY);
		}
		
		FORCEINLINE uint32 GetMemoryIndex() const
		{
			return m_MemoryIndex;
		}

		FORCEINLINE uint32 GetID() const
		{
			return m_ID;
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
				if (pIterator == pBlock)
				{
					return true;
				}
				pIterator = pIterator->pNext;
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
		const GraphicsDeviceVK* const m_pDevice;
		DeviceAllocatorVK* const m_pOwningAllocator;
		const uint32 m_MemoryIndex;
		const uint32 m_ID;
		
		DeviceMemoryBlockVK* m_pHead = nullptr;
		byte* m_pHostMemory = nullptr;
		VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
		uint32 m_MappingCount  = 0;
	};

	/*
	 * DeviceAllocatorVK
	 */

	DeviceAllocatorVK::DeviceAllocatorVK(const GraphicsDeviceVK* pDevice)
		: TDeviceChild(pDevice)
	{
	}

	DeviceAllocatorVK::~DeviceAllocatorVK()
	{
		for (DeviceMemoryPageVK* pMemoryPage : m_Pages)
		{
			SAFEDELETE(pMemoryPage);
		}
	}

	bool DeviceAllocatorVK::Init(const String& debugName, VkDeviceSize pageSize)
	{
		SetName(debugName);

		m_PageSize = pageSize;
		m_DeviceProperties = m_pDevice->GetPhysicalDeviceProperties();
		
		return true;
	}

	bool DeviceAllocatorVK::Allocate(AllocationVK* pAllocation, uint64 sizeInBytes, uint64 alignment, uint32 memoryIndex)
	{
		VALIDATE(pAllocation != nullptr);
		VALIDATE(sizeInBytes > 0);
		
		std::scoped_lock<SpinLock> lock(m_Lock);
		
		// Check if this size every will be possible with this allocator
		VkDeviceSize alignedSize = AlignUp(sizeInBytes, alignment);
		if (alignedSize >= m_PageSize)
		{
			return false;
		}

		if (!m_Pages.IsEmpty())
		{
			for (DeviceMemoryPageVK* pMemoryPage : m_Pages)
			{
				VALIDATE(pMemoryPage != nullptr);

				if (pMemoryPage->GetMemoryIndex() == memoryIndex)
				{
					// Try and allocate otherwise we continue the search
					if (pMemoryPage->Allocate(pAllocation, sizeInBytes, alignment, m_DeviceProperties.limits.bufferImageGranularity))
					{
						return true;
					}
				}
			}
		}
		
		DeviceMemoryPageVK* pNewMemoryPage = DBG_NEW DeviceMemoryPageVK(m_pDevice, this, uint32(m_Pages.GetSize()), memoryIndex);
		if (!pNewMemoryPage->Init(m_PageSize))
		{
			SAFEDELETE(pNewMemoryPage);
			return false;
		}
		else
		{
			SetPageName(pNewMemoryPage);
		}
		
		m_Pages.EmplaceBack(pNewMemoryPage);
		return pNewMemoryPage->Allocate(pAllocation, sizeInBytes, alignment, m_DeviceProperties.limits.bufferImageGranularity);
	}

	bool DeviceAllocatorVK::Free(AllocationVK* pAllocation)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(pAllocation != nullptr);
		DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
		
		VALIDATE(pBlock != nullptr);
		DeviceMemoryPageVK* pPage = pBlock->pPage;
		
		VALIDATE(pPage != nullptr);
		return pPage->Free(pAllocation);
	}

	void* DeviceAllocatorVK::Map(const AllocationVK* pAllocation)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(pAllocation != nullptr);
		DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
		
		VALIDATE(pBlock != nullptr);
		DeviceMemoryPageVK* pPage = pBlock->pPage;
		
		VALIDATE(pPage != nullptr);
		return pPage->Map(pAllocation);
	}

	void DeviceAllocatorVK::Unmap(const AllocationVK* pAllocation)
	{
		std::scoped_lock<SpinLock> lock(m_Lock);

		VALIDATE(pAllocation != nullptr);
		DeviceMemoryBlockVK* pBlock = pAllocation->pBlock;
		
		VALIDATE(pBlock != nullptr);
		DeviceMemoryPageVK* pPage = pBlock->pPage;
		
		VALIDATE(pPage != nullptr);
		return pPage->Unmap(pAllocation);
	}

	void DeviceAllocatorVK::SetPageName(DeviceMemoryPageVK* pMemoryPage)
	{
		VALIDATE(pMemoryPage != nullptr);

		String name = m_DebugName + "[PageID=" + std::to_string(pMemoryPage->GetID()) + "]";
		pMemoryPage->SetName(name);
	}

	void DeviceAllocatorVK::SetName(const String& debugName)
	{
		if (!debugName.empty())
		{
			std::scoped_lock<SpinLock> lock(m_Lock);

			m_DebugName = debugName;
			for (DeviceMemoryPageVK* pMemoryPage : m_Pages)
			{
				SetPageName(pMemoryPage);
			}
		}
	}
}
