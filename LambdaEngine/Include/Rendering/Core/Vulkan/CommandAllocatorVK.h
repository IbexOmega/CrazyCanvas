#pragma once
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class CommandAllocatorVK : public TDeviceChildBase<GraphicsDeviceVK, CommandAllocator>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, CommandAllocator>;

	public:
		CommandAllocatorVK(const GraphicsDeviceVK* pDevice);
		~CommandAllocatorVK();

		bool Init(const String& name, ECommandQueueType queueType);

		VkCommandBuffer AllocateCommandBuffer(VkCommandBufferLevel level);
		void			FreeCommandBuffer(VkCommandBuffer commandBuffer);

	public:
		// DeviceChild Interface
		virtual void SetName(const String& name) override final;
		
		// CommandAllocator Interface
		virtual bool Reset() override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_CommandPool);
		}

	private:
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	};
}