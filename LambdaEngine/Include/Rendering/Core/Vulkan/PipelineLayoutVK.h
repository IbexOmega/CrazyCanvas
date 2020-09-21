#pragma once
#include "Core/TSharedRef.h"

#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorHeapInfo.h"
#include "Rendering/Core/API/TDeviceChildBase.h"
#include "Rendering/Core/API/GraphicsTypes.h"

#include "SamplerVK.h"
#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class PipelineLayoutVK : public TDeviceChildBase<GraphicsDeviceVK, PipelineLayout>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, PipelineLayout>;

		struct DescriptorSetBindingsDesc
		{
			TArray<DescriptorBindingDesc> Bindings;
		};

	public:
		PipelineLayoutVK(const GraphicsDeviceVK* pDevice);
		~PipelineLayoutVK();

		bool Init(const PipelineLayoutDesc* pDesc);

		FORCEINLINE VkPipelineLayout GetPipelineLayout() const
		{
			return m_PipelineLayout;
		}

		FORCEINLINE VkDescriptorSetLayout GetDescriptorSetLayout(uint32 descriptorSetIndex) const
		{
			return m_DescriptorSetLayouts[descriptorSetIndex];
		}

		FORCEINLINE DescriptorHeapInfo GetDescriptorHeapInfo(uint32 descriptorSetIndex) const
		{
			return m_DescriptorCounts[descriptorSetIndex];
		}

		FORCEINLINE TArray<DescriptorBindingDesc> GetDescriptorBindings(uint32 descriptorSetIndex) const
		{
			return m_DescriptorSetBindings[descriptorSetIndex].Bindings;
		}

	public:
		// DeviceChild InterFace
		virtual void SetName(const String& name) override final;

		// PipelineLayout interface
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_PipelineLayout);
		}

	private:
		VkPipelineLayout					m_PipelineLayout = VK_NULL_HANDLE;
		TArray<VkDescriptorSetLayout>		m_DescriptorSetLayouts;
		TArray<DescriptorHeapInfo>			m_DescriptorCounts;
		TArray<DescriptorSetBindingsDesc>	m_DescriptorSetBindings;
		TArray<TSharedRef<Sampler>>			m_ImmutableSamplers;
	};
}
