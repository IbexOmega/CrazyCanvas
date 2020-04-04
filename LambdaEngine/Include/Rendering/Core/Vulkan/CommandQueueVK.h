#pragma once
#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class CommandQueueVK : public DeviceChildBase<GraphicsDeviceVK, ICommandQueue>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ICommandQueue>;

	public:
		CommandQueueVK(const GraphicsDeviceVK* pDevice);
		~CommandQueueVK();

		bool Init(uint32 queueFamilyIndex, uint32 index);

		virtual bool ExecuteCommandLists(const ICommandList* const* ppCommandLists, uint32 numCommandLists, const IFence* pWaitFence) override;
		virtual void Wait() override;

		virtual void SetName(const char* pName) override;

		FORCEINLINE virtual uint64 GetHandle() const override
		{
			return (uint64)m_Queue;
		}

	private:
		VkQueue m_Queue = VK_NULL_HANDLE;
	};
}
