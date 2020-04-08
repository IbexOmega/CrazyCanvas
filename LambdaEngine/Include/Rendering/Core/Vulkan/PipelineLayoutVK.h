#pragma once
#include "Rendering/Core/API/IPipelineLayout.h"
#include "Rendering/Core/API/TDeviceChildBase.h"
#include "Rendering/Core/API/GraphicsTypes.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;

	class PipelineLayoutVK : public TDeviceChildBase<GraphicsDeviceVK, IPipelineLayout>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, IPipelineLayout>;

		struct DescriptorSetLayoutData
		{
			VkDescriptorSetLayoutCreateInfo CreateInfo = { };

		};

	public:
		PipelineLayoutVK(const GraphicsDeviceVK* pDevice);
		~PipelineLayoutVK();

		bool Init(const PipelineLayoutDesc& desc);

		void CreatePushConstantRanges(const ConstantRangeDesc* pConstantRanges, uint32 constantRangeCount, VkPushConstantRange* pResultConstantRanges);
		void CreateDescriptorSetLayouta(const DescriptorSetLayoutDesc* pDescriptorSetLayouts, uint32 descriptorSetLayoutCount, VkDescriptorSetLayoutCreateInfo* pResultDescriptorSetLayouts);

		FORCEINLINE VkPipelineLayout GetPipelineLayout() const
		{
			return m_PipelineLayout;
		}

		FORCEINLINE VkDescriptorSetLayout GetDescriptorSetLayout() const
		{
			return m_DescriptorSetLayout;
		}

		FORCEINLINE DescriptorCountDesc GetDescriptorCount() const
		{
			return m_DescriptorCount;
		}

		// IDeviceChild InterFace
		virtual void SetName(const char* pName) override final;

		// IPipelineLayout interface
		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_PipelineLayout;
		}

	private:
		VkPipelineLayout		m_PipelineLayout		= VK_NULL_HANDLE;
		VkDescriptorSetLayout	m_DescriptorSetLayout	= VK_NULL_HANDLE;
		DescriptorCountDesc		m_DescriptorCount;
	};
}