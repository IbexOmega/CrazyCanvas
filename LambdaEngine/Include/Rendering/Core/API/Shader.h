#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct ShaderDesc
	{
		String				DebugName	= "";
		TArray<byte>		Source;
		String				EntryPoint	= "main";
		FShaderStageFlags	Stage		= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		EShaderLang			Lang		= EShaderLang::SHADER_LANG_NONE;
	};

	struct ShaderReflection
	{
		uint32 NumAtomicCounters	= 0;
		uint32 NumBufferVariables	= 0;
		uint32 NumPipeInputs		= 0;
		uint32 NumPipeOutputs		= 0;
		uint32 NumStorageBuffers	= 0;
		uint32 NumUniformBlocks		= 0;
		uint32 NumUniforms			= 0;
	};

	class Shader : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(Shader);

		/*
		* Returns the API-specific handle to the underlying resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;

		FORCEINLINE const String& GetEntryPoint() const
		{
			return m_Desc.EntryPoint;
		}

		FORCEINLINE const ShaderDesc& GetDesc() const
		{
			return m_Desc;
		}

	protected:
		ShaderDesc m_Desc;
	};
}