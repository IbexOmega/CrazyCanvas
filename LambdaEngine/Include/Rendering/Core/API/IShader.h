#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	struct ShaderDesc
	{
		const char*			pName		= "";
		const char*			pSource		= "";
		uint32				SourceSize	= 0;
		const char*			pEntryPoint	= "main";
		FShaderStageFlags	Stage		= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		EShaderLang			Lang		= EShaderLang::SHADER_LANG_NONE;
	};

	class IShader : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IShader);

		/*
		* Returns the API-specific handle to the underlying resource
		*
		* return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64      GetHandle() const = 0;
		virtual ShaderDesc	GetDesc()   const = 0;
	};
}