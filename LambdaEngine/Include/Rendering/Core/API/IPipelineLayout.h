#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

#define MAX_DESCRIPTOR_BINDINGS		32
#define MAX_CONSTANT_RANGES			16
#define MAX_DESCRIPTOR_SET_LAYOUTS	16
#define MAX_IMMUTABLE_SAMPLERS		32

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
		EDescriptorType DescriptorType				= EDescriptorType::DESCRIPTOR_UNKNOWN;
		uint32			DescriptorCount		= 0;
		uint32			Binding				= 0;
		uint32			ShaderStageMask		= 0;
		ISampler**		ppImmutableSamplers = nullptr;
	};

	struct DescriptorSetLayoutDesc
	{
		const DescriptorBindingDesc* pDescriptorBindings = nullptr;
		uint32 DescriptorBindingCount = 0;
	};

	struct PipelineLayoutDesc
	{
		const char*						pName						= "";
		const DescriptorSetLayoutDesc*	pDescriptorSetLayouts		= nullptr;
		uint32							DescriptorSetLayoutCount	= 0;
		const ConstantRangeDesc*		pConstantRanges				= nullptr;
		uint32							ConstantRangeCount			= 0;
	};

	class IPipelineLayout : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IPipelineLayout);

		virtual uint64 GetHandle() const = 0;
	};
}