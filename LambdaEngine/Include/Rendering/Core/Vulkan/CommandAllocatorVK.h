#pragma once
#include "Rendering/Core/API/ICommandAllocator.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class CommandAllocatorVK : public DeviceChildBase<GraphicsDeviceVK, ICommandAllocator>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ICommandAllocator>;

	public:
		CommandAllocatorVK(const GraphicsDeviceVK* pDevice);
		~CommandAllocatorVK();

		bool Init(const char* pName, ECommandQueueType queueType);

		VkCommandBuffer AllocateCommandBuffer(VkCommandBufferLevel level);

		// IDeviceChild Interface
		virtual void SetName(const char* pName)	override final;
		
		// ICommandAllocator Interface
		virtual bool Reset()					override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_CommandPool;
		}

		FORCEINLINE virtual ECommandQueueType GetType() const override final
		{
			return m_Type;
		}

	private:
		VkCommandPool	m_CommandPool	= VK_NULL_HANDLE;
		ECommandQueueType		m_Type			= ECommandQueueType::COMMAND_QUEUE_UNKNOWN;
	};
}