#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class IShader;
	class IRenderPass;
	class IPipelineLayout;

	union ShaderConstant
	{
		byte	Data[4];
		float32	Float;
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
		PIPELINE_STATE_TYPE_NONE		= 0,
		PIPELINE_STATE_TYPE_GRAPHICS	= 1,
		PIPELINE_STATE_TYPE_COMPUTE		= 2,
		PIPELINE_STATE_TYPE_RAY_TRACING	= 3,
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
		float32			LineWidth					= 1.0f;
		float32			DepthBiasClamp				= 0.0f;
		float32			DepthBiasConstantFactor		= 0.0f;
		float32			DepthBiasSlopeFactor		= 0.0f;
	};

	struct DepthStencilStateDesc
	{
		bool				DepthTestEnable			= true;
		bool				DepthWriteEnable		= true;
		bool				DepthBoundsTestEnable	= false;
		bool				StencilTestEnable		= false;
		float32				MinDepthBounds			= 0.0f;
		float32				MaxDepthBounds			= 1.0f;
		ECompareOp			CompareOp				= ECompareOp::COMPARE_OP_LESS_OR_EQUAL;
		StencilOpStateDesc	FrontFace				= { };
		StencilOpStateDesc	BackFace				= { };
	};

	struct BlendAttachmentStateDesc
	{
		bool			BlendEnabled				= false;
		uint32			RenderTargetComponentMask	= FColorComponentFlags::COLOR_COMPONENT_FLAG_NONE;
		EBlendOp		BlendOp						= EBlendOp::BLEND_OP_ADD;
		EBlendFactor	SrcBlend					= EBlendFactor::BLEND_FACTOR_ONE;
		EBlendFactor	DstBlend					= EBlendFactor::BLEND_FACTOR_ZERO;
		EBlendOp		BlendOpAlpha				= EBlendOp::BLEND_OP_ADD;
		EBlendFactor	SrcBlendAlpha				= EBlendFactor::BLEND_FACTOR_ONE;
		EBlendFactor	DstBlendAlpha				= EBlendFactor::BLEND_FACTOR_ZERO;
	};

	struct BlendStateDesc
	{
		const BlendAttachmentStateDesc*	pBlendAttachmentStates		= nullptr;
		uint32							BlendAttachmentStateCount	= 0;
		bool							AlphaToCoverageEnable	= true;
		bool							AlphaToOneEnable		= true;
		bool							LogicOpEnable			= false;
		ELogicOp						LogicOp					= ELogicOp::LOGIC_OP_NO_OP;
		float32							BlendConstants[4];
	};

	struct VertexInputAttributeDesc
	{
		uint32	Location = 0;
		uint32	Offset	 = 0;
		EFormat	Format	 = EFormat::FORMAT_NONE;
	};

	struct VertexInputBindingDesc
	{
		uint32						Binding			= 0;
		uint32						Stride			= 0;
		EVertexInputRate			InputRate		= EVertexInputRate::VERTEX_INPUT_NONE;
		VertexInputAttributeDesc	pAttributes[MAX_ATTRIBUTES_PER_VERTEX];
		uint32						AttributeCount	= 0;
	};

	struct GraphicsPipelineStateDesc
	{
		const char* pName = "";
		
		const IRenderPass*		pRenderPass		= nullptr;
		const IPipelineLayout*	pPipelineLayout	= nullptr;

		VertexInputBindingDesc	pVertexInputBindings[MAX_VERTEX_INPUT_ATTACHMENTS];
		uint32					VertexInputBindingCount	= 0;

		InputAssemblyDesc		InputAssembly		= { };
		DepthStencilStateDesc	DepthStencilState	= { };
		BlendStateDesc			BlendState			= { };
		RasterizerStateDesc		RasterizerState		= { };
		uint32					SampleMask			= 0;
		uint32					SampleCount			= 1;
		uint32					Subpass				= 0;

		// "New Style"
		const ShaderModuleDesc* pTaskShader = nullptr;
		const ShaderModuleDesc* pMeshShader = nullptr;
		// "Old style"
		const ShaderModuleDesc* pVertexShader	= nullptr;
		const ShaderModuleDesc* pHullShader		= nullptr;
		const ShaderModuleDesc* pDomainShader	= nullptr;
		const ShaderModuleDesc* pGeometryShader	= nullptr;
		// Common
		const ShaderModuleDesc* pPixelShader	= nullptr;
	};

	struct ComputePipelineStateDesc
	{
		const char*				pName			= "";
		const IPipelineLayout*	pPipelineLayout	= nullptr;
		const ShaderModuleDesc*	pShader			= nullptr;
	};

	struct RayTracingPipelineStateDesc
	{
		const char*				pName				= "";
		const IPipelineLayout*	pPipelineLayout		= nullptr;
		uint32					MaxRecursionDepth	= 1;	

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
