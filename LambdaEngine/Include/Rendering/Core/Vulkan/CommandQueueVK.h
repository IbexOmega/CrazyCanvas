#pragma once
#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

#define MAX_COMMANDBUFFERS 32

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

		virtual bool ExecuteCommandLists(const ICommandList* const* ppCommandLists, uint32 numCommandLists, FPipelineStageFlags waitStage, const IFence* pWaitFence, uint64 waitValue, const IFence* pSignalFence, uint64 signalValue) override final;
		virtual void Flush() override final;

		virtual void SetName(const char* pName) override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_Queue;
		}

	private:
		VkQueue			m_Queue = VK_NULL_HANDLE;
		VkCommandBuffer m_CommandBuffers[MAX_COMMANDBUFFERS];
	};
}
