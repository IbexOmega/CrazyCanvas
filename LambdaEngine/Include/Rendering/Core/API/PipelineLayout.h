#pragma once
#include "Sampler.h"
#include "GraphicsTypes.h"

#include "Core/TSharedRef.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	struct ConstantRangeDesc
	{
		uint32 ShaderStageFlags = 0;
		uint32 SizeInBytes		= 0;
		uint32 OffsetInBytes	= 0;
	};

	struct DescriptorBindingDesc
	{
		EDescriptorType						DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
		uint32								DescriptorCount		= 0;
		uint32								Binding				= 0;
		uint32								ShaderStageMask		= 0;
		FDescriptorSetLayoutBindingFlags	Flags				= FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_NONE;
		TArray<TSharedRef<Sampler>>	ImmutableSamplers;
	};

	struct DescriptorSetLayoutDesc
	{
		FDescriptorSetLayoutsFlags		DescriptorSetLayoutFlags = FDescriptorSetLayoutsFlag::DESCRIPTOR_SET_LAYOUT_FLAG_NONE;
		TArray<DescriptorBindingDesc>	DescriptorBindings;
	};

	struct PipelineLayoutDesc
	{
		String							DebugName				= "";
		TArray<DescriptorSetLayoutDesc>	DescriptorSetLayouts;
		TArray<ConstantRangeDesc>		ConstantRanges;
	};

	class PipelineLayout : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(PipelineLayout);

		/*
		* Returns the API-specific handle to the underlaying texture-resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;

		FORCEINLINE const PipelineLayoutDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		PipelineLayoutDesc m_Desc;
	};
}