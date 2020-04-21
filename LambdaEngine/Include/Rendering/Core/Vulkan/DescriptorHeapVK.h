#pragma once
#include "Rendering/Core/API/IDescriptorHeap.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class DescriptorHeapVK : public TDeviceChildBase<GraphicsDeviceVK, IDescriptorHeap>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IDescriptorHeap>;

	public:
		DescriptorHeapVK(const GraphicsDeviceVK* pDevice);
		~DescriptorHeapVK();

		bool Init(const DescriptorHeapDesc* pDesc);

		VkDescriptorSet AllocateDescriptorSet(const IPipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex);
		void			FreeDescriptorSet(VkDescriptorSet descriptorSet);

		// IDeviceChild interface
		virtual void SetName(const char* pName) override final;

		// IDescriptorHeap interface
		FORCEINLINE virtual DescriptorCountDesc GetHeapStatus() const override final
		{
			return m_HeapStatus;
		}

		FORCEINLINE virtual DescriptorHeapDesc GetDesc() const override final
		{
			return m_Desc;
		}

	private:
		VkDescriptorPool	m_DescriptorHeap;
		DescriptorCountDesc m_HeapStatus;
		DescriptorHeapDesc	m_Desc;
	};
}
