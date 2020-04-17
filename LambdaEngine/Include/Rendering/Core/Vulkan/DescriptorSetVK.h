#pragma once
#include "Rendering/Core/API/IDescriptorSet.h"
#include "Rendering/Core/API/IPipelineLayout.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class IPipelineLayout;
	class GraphicsDeviceVK;
	class DescriptorHeapVK;

	class DescriptorSetVK : public TDeviceChildBase<GraphicsDeviceVK, IDescriptorSet>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IDescriptorSet>;

	public:
		DescriptorSetVK(const GraphicsDeviceVK* pDevice);
		~DescriptorSetVK();

		bool Init(const char* pName, const IPipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, IDescriptorHeap* pDescriptorHeap);

		FORCEINLINE VkDescriptorSet GetDescriptorSet() const
		{
			return m_DescriptorSet;
		}

		FORCEINLINE DescriptorBindingDesc GetDescriptorBindingDesc(uint32 bindingIndex) const
		{
			return m_Bindings[bindingIndex];
		}

		FORCEINLINE uint32 GetDescriptorBindingDescCount() const
		{
			return m_BindingCount;
		}

		// IDeviceChild Interface
		virtual void SetName(const char* pName) override final;

		// IDesciptorSet Interface
		virtual void WriteTextureDescriptors(const ITextureView* const* ppTextures, const ISampler* const* ppSamplers, ETextureState textureState, uint32 firstBinding, uint32 descriptorCount, EDescriptorType type) override final;
		virtual void WriteBufferDescriptors(const IBuffer* const* ppBuffers, const uint32* pOffsets, const uint32* pSizes, uint32 firstBinding, uint32 descriptorCount, EDescriptorType type)	override final;
		virtual void WriteAccelerationStructureDescriptors(uint32 firstBinding, uint32 descriptorCount) override final;

		virtual IDescriptorHeap* GetHeap() override final;
		
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_pDescriptorHeap;
		}

	private:
		VkDescriptorSet			m_DescriptorSet		= VK_NULL_HANDLE;
		DescriptorHeapVK*		m_pDescriptorHeap	= nullptr;
		
		DescriptorBindingDesc	m_Bindings[MAX_DESCRIPTOR_BINDINGS];
		uint32					m_BindingCount	= 0;
	};
}