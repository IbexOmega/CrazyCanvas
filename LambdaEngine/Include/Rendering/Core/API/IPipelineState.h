#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	constexpr uint32 MAX_CLOSEST_HIT_SHADER_COUNT	= 8;
	constexpr uint32 MAX_MISS_SHADER_COUNT			= 8;

	class IShader;
	class IRenderPass;
	class IPipelineLayout;

	union ShaderConstant
	{
		byte	Data[4];
		float	Float;
		int32	Integer;
	};

	struct ShaderModuleDesc
	{
		IShader*				pShader				= nullptr;
		const ShaderConstant*	pConstants			= nullptr;
		uint32					ShaderConstantCount = 0;
	};

	enum class EPipelineStateType : uint8
	{
		NONE					= 0,
		PIPELINE_GRAPHICS       = 1,
		PIPELINE_COMPUTE        = 2,
		PIPELINE_RAY_TRACING    = 3,
	};

	struct InputAssemblyDesc
	{
		EPrimitiveTopology	PrimitiveTopology		= EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		bool				PrimitiveRestartEnable	= false;
	};

	struct RasterizerStateDesc
	{
		EPolygonMode	PolygonMode					= EPolygonMode::POLYGON_MODE_FILL;
		ECullMode		CullMode					= ECullMode::CULL_MODE_NONE;
		bool			FrontFaceCounterClockWise	= false;
		bool			RasterizerDiscardEnable		= false;
		bool			DepthBiasEnable				= false;
		bool			DepthClampEnable			= false;
		bool			MultisampleEnable			= false;
		float			LineWidth					= 1.0f;
		float			DepthBiasClamp				= 0.0f;
		float			DepthBiasConstantFactor		= 0.0f;
		float			DepthBiasSlopeFactor		= 0.0f;
	};

	struct DepthStencilStateDesc
	{

	};

	struct BlendAttachmentStateDesc
	{
		bool			BlendEnabled				= false;
		uint32			RenderTargetComponentsMask	= FColorComponentFlags::COLOR_COMPONENT_FLAG_NONE;
		EBlendOp		BlendOp						= EBlendOp::BLEND_OP_ADD;
		EBlendFactor	SrcBlend					= EBlendFactor::BLEND_FACTOR_ONE;
		EBlendFactor	DstBlend					= EBlendFactor::BLEND_FACTOR_ZERO;
		EBlendOp		BlendOpAlpha				= EBlendOp::BLEND_OP_ADD;
		EBlendFactor	SrcBlendAlpha				= EBlendFactor::BLEND_FACTOR_ONE;
		EBlendFactor	DstBlendAlpha				= EBlendFactor::BLEND_FACTOR_ZERO;
	};

	struct BlendStateDesc
	{
		bool		AlphaToCoverageEnable	= true;
		bool		AlphaToOneEnable		= true;
		bool		LogicOpEnable			= false;
		ELogicOp	LogicOp					= ELogicOp::LOGIC_OP_NOOP;

		const BlendAttachmentStateDesc*	pBlendAttachmentStates		= nullptr;
		uint32							BlendAttachmentStateCount	= 0;

		float BlendConstants[4];
	};

	struct GraphicsPipelineStateDesc
	{
		const char* pName = "";
		
		const IRenderPass*		pRenderPass		= nullptr;
		const IPipelineLayout*	pPipelineLayout	= nullptr;

		InputAssemblyDesc		InputAssembly		= { };
		DepthStencilStateDesc	DepthStencilState	= { };
		BlendStateDesc			BlendState			= { };
		RasterizerStateDesc		RasterizerState		= { };
		uint32					SampleMask			= 0;
		uint32					SampleCount			= 1;
		uint32					Subpass				= 0;

		// "New Style"
		const ShaderModuleDesc* pTaskShader;
		const ShaderModuleDesc* pMeshShader;
		// "Old style"
		const ShaderModuleDesc* pVertexShader;
		const ShaderModuleDesc* pHullShader;
		const ShaderModuleDesc* pDomainShader;
		const ShaderModuleDesc* pGeometryShader;
		// Common
		const ShaderModuleDesc* PixelShader;
	};

	struct ComputePipelineStateDesc
	{
		const char*				pName			= "";
		const IPipelineLayout*	pPipelineLayout	= nullptr;
		const ShaderModuleDesc*	pShader			= nullptr;
	};

	struct RayTracingPipelineStateDesc
	{
		const char*				pName					= "";
		const IPipelineLayout*	pPipelineLayout			= nullptr;
		uint32					MaxRecursionDepth		= 1;	

		const ShaderModuleDesc*		pRaygenShader			= nullptr;
		const ShaderModuleDesc**	ppMissShaders			= nullptr;
		uint32						MissShaderCount			= 0;
		const ShaderModuleDesc**	ppClosestHitShaders		= nullptr; 
		uint32						ClosestHitShaderCount	= 0;
	};

	class IPipelineState : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(IPipelineState);

		virtual EPipelineStateType GetType() const = 0;
	};
}
