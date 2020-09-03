#pragma once
#include "Core/TSharedRef.h"

#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Containers/String.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class PipelineLayout;
	class GraphicsDeviceVK;
	class DescriptorHeapVK;

	class DescriptorSetVK : public TDeviceChildBase<GraphicsDeviceVK, DescriptorSet>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, DescriptorSet>;

	public:
		DescriptorSetVK(const GraphicsDeviceVK* pDevice);
		~DescriptorSetVK();

		bool Init(const String& name, const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, DescriptorHeap* pDescriptorHeap);

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
			return m_Bindings.GetSize();
		}

	public:
		// DeviceChild Interface
		virtual void SetName(const String& name) override final;

		// DesciptorSet Interface
		virtual void WriteTextureDescriptors(const TextureView* const* ppTextures, const Sampler* const* ppSamplers, ETextureState textureState, uint32 firstBinding, uint32 descriptorCount, EDescriptorType type) override final;
		virtual void WriteBufferDescriptors(const Buffer* const* ppBuffers, const uint64* pOffsets, const uint64* pSizes, uint32 firstBinding, uint32 descriptorCount, EDescriptorType type)	override final;
		virtual void WriteAccelerationStructureDescriptors(const AccelerationStructure* const * ppAccelerationStructures, uint32 firstBinding, uint32 descriptorCount) override final;

		virtual DescriptorHeap* GetHeap() override final;
		
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_DescriptorSet);
		}

	private:
		VkDescriptorSet					m_DescriptorSet		= VK_NULL_HANDLE;
		TSharedRef<DescriptorHeapVK>	m_DescriptorHeap	= nullptr;
		TArray<DescriptorBindingDesc>	m_Bindings;
	};
}
