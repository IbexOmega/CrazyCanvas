#pragma once
#include "Threading/API/SpinLock.h"

#include "Containers/TArray.h"

#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class DeviceMemoryPageVK;

	struct DeviceMemoryBlockVK;

	struct AllocationVK
	{
		DeviceMemoryBlockVK* pBlock = nullptr;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		uint64 Offset = 0;
		class DeviceAllocatorVK* pAllocator = nullptr;
	};

	class DeviceAllocatorVK : public TDeviceChildBase<GraphicsDeviceVK, DeviceChild>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, DeviceChild>;
		
	public:
		DeviceAllocatorVK(const GraphicsDeviceVK* pDevice);
		~DeviceAllocatorVK();
		
		bool Init(const String& debugName, VkDeviceSize pageSize);
		
		bool Allocate(AllocationVK* pAllocation, uint64 sizeInBytes, uint64 alignment, uint32 memoryIndex);
		bool Free(AllocationVK* pAllocation);
		
		void* Map(const AllocationVK* pAllocation);
		void Unmap(const AllocationVK* pAllocation);

	public:
		// DeviceChild Interface
		virtual void SetName(const String& name) override final;

	private:
		void SetPageName(DeviceMemoryPageVK* pMemoryPage);
		
	private:
		TArray<DeviceMemoryPageVK*> m_Pages;
		VkPhysicalDeviceProperties m_DeviceProperties;
		VkDeviceSize m_PageSize;
		String m_DebugName;
		SpinLock m_Lock;
	};
}
