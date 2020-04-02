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

		bool Init(EQueueType queueType);

		virtual bool Reset() override;
		
		virtual void SetName(const char* pName)	override;

		FORCEINLINE virtual uint64 GetHandle() const override
		{
			return (uint64)m_CommandPool;
		}

		FORCEINLINE virtual EQueueType GetType() const override
		{
			return m_Type;
		}

	private:
		VkCommandPool	m_CommandPool	= VK_NULL_HANDLE;
		EQueueType		m_Type			= EQueueType::QUEUE_UNKNOWN;
	};
}