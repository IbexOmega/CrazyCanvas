#pragma once

#include "IDeviceChild.h"
#include "GraphicsTypes.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	constexpr uint32 MAX_CLOSEST_HIT_SHADER_COUNT	= 8;
	constexpr uint32 MAX_MISS_SHADER_COUNT			= 8;

	class IShader;
	class IRenderPass;
	class IPipelineLayout;

	enum class EPipelineStateType : uint8
	{
		NONE            = 0,
		GRAPHICS        = 1,
		COMPUTE         = 2,
		RAY_TRACING     = 3,
	};

	struct BlendAttachmentState
	{
		bool BlendEnabled				= false;
		uint32 ColorComponentsMask		= FColorComponentFlags::COLOR_COMPONENT_FLAG_NONE;
	};

	struct GraphicsPipelineStateDesc
	{
		const char* pName									= "";
		const IRenderPass* pRenderPass						= nullptr;
		const IPipelineLayout* pPipelineLayout				= nullptr;
		const BlendAttachmentState* pBlendAttachmentStates	= nullptr;
		uint32 BlendAttachmentStateCount					= 0;

		//New Style
		const IShader* pTaskShader							= nullptr;
		const IShader* pMeshShader							= nullptr;

		//Old Style
		const IShader* pVertexShader						= nullptr;
		const IShader* pGeometryShader						= nullptr;
		const IShader* pHullShader							= nullptr;
		const IShader* pDomainShader						= nullptr;

		//Both
		const IShader* pPixelShader							= nullptr;
	};

	struct ComputePipelineStateDesc
	{
		const char*				pName				= "";
		const IPipelineLayout*	pPipelineLayout		= nullptr;
		const IShader*			pShader				= nullptr;
	};

	struct RayTracingPipelineStateDesc
	{
		const char*				pName					= "";
		const IPipelineLayout*	pPipelineLayout			= nullptr;
		uint32					MaxRecursionDepth		= 1;	

		const IShader*			pRaygenShader			= nullptr;
		const IShader* 			ppMissShaders[MAX_MISS_SHADER_COUNT];
		const IShader*			ppClosestHitShaders[MAX_CLOSEST_HIT_SHADER_COUNT]; 

		uint32					MissShaderCount			= 0;
		uint32					ClosestHitShaderCount	= 0;
	};

	class IPipelineState : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IPipelineState);

		virtual EPipelineStateType GetType() const = 0;
	};
}
