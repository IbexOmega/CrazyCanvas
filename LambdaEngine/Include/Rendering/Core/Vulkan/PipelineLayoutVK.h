#pragma once
#include "Rendering/Core/API/IPipelineLayout.h"
#include "Rendering/Core/API/TDeviceChildBase.h"
#include "Rendering/Core/API/GraphicsTypes.h"

#include "Vulkan.h"

#define MAX_TOTAL_IMMUTABLE_SAMPLERS (MAX_DESCRIPTOR_SET_LAYOUTS * MAX_DESCRIPTOR_BINDINGS * MAX_IMMUTABLE_SAMPLERS)

namespace LambdaEngine
{
	class SamplerVK;
	class GraphicsDeviceVK;

	struct DescriptorSetBindingsDesc
	{
		DescriptorBindingDesc	Bindings[MAX_DESCRIPTOR_BINDINGS];
		uint32					BindingCount = 0;
	};

	class PipelineLayoutVK : public TDeviceChildBase<GraphicsDeviceVK, IPipelineLayout>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IPipelineLayout>;

		struct ImmutableSamplersData
		{
			VkSampler ImmutableSamplers[MAX_IMMUTABLE_SAMPLERS];
		};

		struct DescriptorSetLayoutData
		{
			VkDescriptorSetLayoutCreateInfo CreateInfo				= { };
			uint32							DescriptorBindingCount	= 0;
			VkDescriptorSetLayoutBinding	DescriptorBindings[MAX_DESCRIPTOR_BINDINGS];
			ImmutableSamplersData			ImmutableSamplers[MAX_DESCRIPTOR_BINDINGS];
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
			ASSERT(descriptorSetIndex < MAX_DESCRIPTOR_SET_LAYOUTS);
			return m_DescriptorSetLayouts[descriptorSetIndex];
		}

		FORCEINLINE DescriptorCountDesc GetDescriptorCount(uint32 descriptorSetIndex) const
		{
			ASSERT(descriptorSetIndex < MAX_DESCRIPTOR_SET_LAYOUTS);
			return m_DescriptorCounts[descriptorSetIndex];
		}

		FORCEINLINE DescriptorSetBindingsDesc GetDescriptorBindings(uint32 descriptorSetIndex) const
		{
			ASSERT(descriptorSetIndex < MAX_DESCRIPTOR_SET_LAYOUTS);
			return m_DescriptorSetBindings[descriptorSetIndex];
		}

		// IDeviceChild InterFace
		virtual void SetName(const char* pName) override final;

		// IPipelineLayout interface
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_PipelineLayout;
		}

	private:
		void CreatePushConstantRanges(const ConstantRangeDesc* pConstantRanges, uint32 constantRangeCount, VkPushConstantRange* pResultConstantRanges);
		void CreateDescriptorSetLayout(const DescriptorSetLayoutDesc* pDescriptorSetLayouts, uint32 descriptorSetLayoutCount, DescriptorSetLayoutData* pResultDescriptorSetLayouts);

	private:
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		VkDescriptorSetLayout	    m_DescriptorSetLayouts[MAX_DESCRIPTOR_SET_LAYOUTS];
		DescriptorCountDesc		    m_DescriptorCounts[MAX_DESCRIPTOR_SET_LAYOUTS];
		DescriptorSetBindingsDesc	m_DescriptorSetBindings[MAX_DESCRIPTOR_SET_LAYOUTS];
		uint32					    m_DescriptorSetCount = 0;
		
		uint32		m_ImmutableSamplerCount = 0;
		SamplerVK*	m_ppImmutableSamplers[MAX_TOTAL_IMMUTABLE_SAMPLERS];
	};
}
