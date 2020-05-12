#pragma once

#include "IDeviceChild.h"
#include "GraphicsTypes.h"

#include "Rendering/Core/API/IRenderPass.h"

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

	struct VertexInputAttributeDesc
	{
		uint32		Location	=	0;
		uint32		Offset		=	0;
		EFormat		Format		=	EFormat::NONE;
	};

	struct VertexInputBindingDesc
	{
		uint32						Binding			= 0;
		uint32						Stride			= 0;
		EVertexInputRate			InputRate		= EVertexInputRate::NONE;
		VertexInputAttributeDesc*	pAttributes		= nullptr;
		uint32						AttributeCount	= 0;
	};

	struct GraphicsPipelineStateDesc
	{
		const char*					pName						= "";
		const IRenderPass*			pRenderPass					= nullptr;
		const IPipelineLayout*		pPipelineLayout				= nullptr;

		VertexInputBindingDesc*		pVertexInputBindings		= nullptr;
		uint32						VertexInputBindingCount		= 0;
		
		BlendAttachmentState		pBlendAttachmentStates		[MAX_COLOR_ATTACHMENTS];
		uint32						BlendAttachmentStateCount	= 0;

		//New Style
		const IShader*				pTaskShader					= nullptr;
		const IShader*				pMeshShader					= nullptr;

		//Old Style
		const IShader*				pVertexShader				= nullptr;
		const IShader*				pGeometryShader				= nullptr;
		const IShader*				pHullShader					= nullptr;
		const IShader*				pDomainShader				= nullptr;

		//Both
		const IShader*				pPixelShader				= nullptr;
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
