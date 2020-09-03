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