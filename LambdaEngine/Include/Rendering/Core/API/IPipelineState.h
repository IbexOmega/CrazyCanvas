#pragma once

#include "IDeviceChild.h"
#include "GraphicsTypes.h"

#include <vector>

namespace LambdaEngine
{
	enum class EPipelineStateType : uint8
	{
		NONE            = 0,
		GRAPHICS        = 1,
		COMPUTE         = 2,
		RAY_TRACING     = 3
	};

	struct GraphicsPipelineDesc
	{
		const char* pName			= "";
		ShaderDesc MeshShader		= {};
		ShaderDesc VertexShader		= {};
		ShaderDesc GeometryShader	= {};
		ShaderDesc HullShader		= {};
		ShaderDesc DomainShader		= {};
		ShaderDesc PixelShader		= {};
	};

	struct ComputePipelineDesc
	{
		const char* pName			= "";
		ShaderDesc Shader			= {};
	};

	struct RayTracingPipelineDesc
	{
		const char* pName			= "";
		uint32 MaxRecursionDepth	= 1;	
		ShaderDesc RaygenShader		= {};
		std::vector<ShaderDesc> MissShaders;
		std::vector<ShaderDesc> ClosestHitShaders;
	};

	class IPipelineState : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IPipelineState);

		virtual EPipelineStateType GetType() const = 0;
	};
}
