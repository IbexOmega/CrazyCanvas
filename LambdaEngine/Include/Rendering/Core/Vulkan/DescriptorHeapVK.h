#pragma once
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class DescriptorHeapVK : public TDeviceChildBase<GraphicsDeviceVK, DescriptorHeap>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, DescriptorHeap>;

	public:
		DescriptorHeapVK(const GraphicsDeviceVK* pDevice);
		~DescriptorHeapVK();

		bool Init(const DescriptorHeapDesc* pDesc);

		VkDescriptorSet AllocateDescriptorSet(const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex);
		void			FreeDescriptorSet(VkDescriptorSet descriptorSet);

	public:
		// DescriptorHeap Interface
		virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_DescriptorHeap);
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override final;

	private:
		VkDescriptorPool m_DescriptorHeap;
	};
}
