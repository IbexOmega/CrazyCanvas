#pragma once
#include "Rendering/Core/API/IQueue.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class QueueVK : public DeviceChildBase<GraphicsDeviceVK, IQueue>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, IQueue>;

	public:
		QueueVK(const GraphicsDeviceVK* pDevice);
		~QueueVK();

		bool Init(uint32 queueFamilyIndex, uint32 index);

		virtual bool ExecuteCommandList(const ICommandList* const* ppCommandList, uint32 numCommandLists, const IFence* pFence) override;
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