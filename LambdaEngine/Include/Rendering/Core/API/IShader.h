#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	union ShaderConstant
	{
		byte	Data[4];
		float	Float;
		int32	Integer;
	};

	struct ShaderDesc
	{
		String				Name			= "";
		const char*			pSource			= "";
		uint32				SourceSize		= 0;
		const char*			pEntryPoint		= "main";
		FShaderStageFlags	Stage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
		EShaderLang			Lang			= EShaderLang::NONE;

		ShaderConstant* pConstants		= nullptr;
		uint32 ShaderConstantCount		= 0;
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