#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class DescriptorHeapInfo
	{
	public:
		DescriptorHeapInfo()									= default;
		DescriptorHeapInfo(const DescriptorHeapInfo& other)		= default;
		DescriptorHeapInfo(DescriptorHeapInfo&& other)			= default;
		~DescriptorHeapInfo()									= default;

		DescriptorHeapInfo& operator=(DescriptorHeapInfo&& other)		= default;
		DescriptorHeapInfo& operator=(const DescriptorHeapInfo& other)	= default;

		inline DescriptorHeapInfo& operator+=(const DescriptorHeapInfo& other)
		{
			*this = *this + other;
			return *this;
		}

		inline DescriptorHeapInfo& operator-=(const DescriptorHeapInfo& other)
		{
			*this = *this - other;
			return *this;
		}

		inline bool IsValid() const
		{
			return
				SamplerDescriptorCount					>= 0 &&
				TextureDescriptorCount					>= 0 &&
				TextureCombinedSamplerDescriptorCount	>= 0 &&
				ConstantBufferDescriptorCount			>= 0 &&
				UnorderedAccessBufferDescriptorCount	>= 0 &&
				UnorderedAccessTextureDescriptorCount	>= 0 &&
				AccelerationStructureDescriptorCount	>= 0;
		}

		inline bool operator==(const DescriptorHeapInfo& other) const
		{
			return 
				SamplerDescriptorCount					== other.SamplerDescriptorCount					&&
				TextureDescriptorCount					== other.TextureDescriptorCount					&&
				TextureCombinedSamplerDescriptorCount	== other.TextureCombinedSamplerDescriptorCount	&&
				ConstantBufferDescriptorCount			== other.ConstantBufferDescriptorCount			&&
				UnorderedAccessBufferDescriptorCount	== other.UnorderedAccessBufferDescriptorCount	&&
				UnorderedAccessTextureDescriptorCount	== other.UnorderedAccessTextureDescriptorCount	&&
				AccelerationStructureDescriptorCount	== other.AccelerationStructureDescriptorCount;
		}

		inline bool operator!=(const DescriptorHeapInfo& other) const
		{
			return !(*this == other);
		}

		inline friend DescriptorHeapInfo operator-(const DescriptorHeapInfo& left, const DescriptorHeapInfo& right)
		{
			DescriptorHeapInfo result;

			result.SamplerDescriptorCount					= left.SamplerDescriptorCount				 - right.SamplerDescriptorCount;
			result.TextureDescriptorCount					= left.TextureDescriptorCount				 - right.TextureDescriptorCount;
			result.TextureCombinedSamplerDescriptorCount	= left.TextureCombinedSamplerDescriptorCount - right.TextureCombinedSamplerDescriptorCount;
			result.ConstantBufferDescriptorCount			= left.ConstantBufferDescriptorCount		 - right.ConstantBufferDescriptorCount;
			result.UnorderedAccessBufferDescriptorCount		= left.UnorderedAccessBufferDescriptorCount	 - right.UnorderedAccessBufferDescriptorCount;
			result.UnorderedAccessTextureDescriptorCount	= left.UnorderedAccessTextureDescriptorCount - right.UnorderedAccessTextureDescriptorCount;
			result.AccelerationStructureDescriptorCount		= left.AccelerationStructureDescriptorCount	 - right.AccelerationStructureDescriptorCount;
			
			return result;
		}

		inline friend DescriptorHeapInfo operator+(const DescriptorHeapInfo& left, const DescriptorHeapInfo& right)
		{
			DescriptorHeapInfo result;

			result.SamplerDescriptorCount					= left.SamplerDescriptorCount				 + right.SamplerDescriptorCount;
			result.TextureDescriptorCount					= left.TextureDescriptorCount				 + right.TextureDescriptorCount;
			result.TextureCombinedSamplerDescriptorCount	= left.TextureCombinedSamplerDescriptorCount + right.TextureCombinedSamplerDescriptorCount;
			result.ConstantBufferDescriptorCount			= left.ConstantBufferDescriptorCount		 + right.ConstantBufferDescriptorCount;
			result.UnorderedAccessBufferDescriptorCount		= left.UnorderedAccessBufferDescriptorCount	 + right.UnorderedAccessBufferDescriptorCount;
			result.UnorderedAccessTextureDescriptorCount	= left.UnorderedAccessTextureDescriptorCount + right.UnorderedAccessTextureDescriptorCount;
			result.AccelerationStructureDescriptorCount		= left.AccelerationStructureDescriptorCount	 + right.AccelerationStructureDescriptorCount;

			return result;
		}

	public:
		uint32 SamplerDescriptorCount					= 0;
		uint32 TextureDescriptorCount					= 0;
		uint32 TextureCombinedSamplerDescriptorCount	= 0;
		uint32 ConstantBufferDescriptorCount			= 0;
		uint32 UnorderedAccessBufferDescriptorCount		= 0;
		uint32 UnorderedAccessTextureDescriptorCount	= 0;
		uint32 AccelerationStructureDescriptorCount		= 0;
	};
}