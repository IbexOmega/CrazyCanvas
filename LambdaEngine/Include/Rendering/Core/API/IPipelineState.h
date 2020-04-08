#pragma once

#include "IDeviceChild.h"
#include "GraphicsTypes.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	enum class EPipelineStateType : uint8
	{
		NONE            = 0,
		GRAPHICS        = 1,
		COMPUTE         = 2,
		RAY_TRACING     = 3
	};

	struct GraphicsPipelineStateDesc
	{
		const char* pName			= "";
		ShaderDesc	MeshShader		= { };
		ShaderDesc	VertexShader	= { };
		ShaderDesc	GeometryShader	= { };
		ShaderDesc	HullShader		= { };
		ShaderDesc	DomainShader	= { };
		ShaderDesc	PixelShader		= { };
	};

	struct ComputePipelineStateDesc
	{
		const char* pName	= "";
		ShaderDesc	Shader	= { };
	};

	struct RayTracingPipelineStateDesc
	{
		const char*			pName					= "";
		uint32				MaxRecursionDepth		= 1;	
		ShaderDesc			RaygenShader			= {};
		const ShaderDesc*	pMissShaders			= nullptr;
		uint32				MissShaderCount			= 0;
		const ShaderDesc*	pClosestHitShaders		= nullptr;
		uint32				ClosestHitShaderCount	= 0;
	};

	class IPipelineState : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IPipelineState);

		virtual EPipelineStateType GetType() const = 0;
	};
}
