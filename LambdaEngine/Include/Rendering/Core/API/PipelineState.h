#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class Shader;
	class RenderPass;
	class PipelineLayout;

	union ShaderConstant
	{
		byte	Data[4];
		float32	Float;
		int32	Integer;
	};

	struct ShaderModuleDesc
	{
		Shader*					pShader	= nullptr;
		TArray<ShaderConstant>	ShaderConstants;
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
		float32			LineWidth					= 1.0f;
		float32			DepthBiasClamp				= 0.0f;
		float32			DepthBiasConstantFactor		= 0.0f;
		float32			DepthBiasSlopeFactor		= 0.0f;
		bool			FrontFaceCounterClockWise	= false;
		bool			RasterizerDiscardEnable		= false;
		bool			DepthBiasEnable				= false;
		bool			DepthClampEnable			= false;
		bool			MultisampleEnable			= false;
	};

	struct DepthStencilStateDesc
	{
		ECompareOp			CompareOp				= ECompareOp::COMPARE_OP_LESS_OR_EQUAL;
		StencilOpStateDesc	FrontFace				= { };
		StencilOpStateDesc	BackFace				= { };
		float32				MinDepthBounds			= 0.0f;
		float32				MaxDepthBounds			= 1.0f;
		bool				DepthTestEnable			= true;
		bool				DepthWriteEnable		= true;
		bool				DepthBoundsTestEnable	= false;
		bool				StencilTestEnable		= false;
	};

	struct BlendAttachmentStateDesc
	{
		EBlendOp		BlendOp						= EBlendOp::BLEND_OP_ADD;
		EBlendFactor	SrcBlend					= EBlendFactor::BLEND_FACTOR_ONE;
		EBlendFactor	DstBlend					= EBlendFactor::BLEND_FACTOR_ZERO;
		EBlendOp		BlendOpAlpha				= EBlendOp::BLEND_OP_ADD;
		EBlendFactor	SrcBlendAlpha				= EBlendFactor::BLEND_FACTOR_ONE;
		EBlendFactor	DstBlendAlpha				= EBlendFactor::BLEND_FACTOR_ZERO;
		uint32			RenderTargetComponentMask	= FColorComponentFlags::COLOR_COMPONENT_FLAG_NONE;
		bool			BlendEnabled				= false;
	};

	struct BlendStateDesc
	{
		TArray<BlendAttachmentStateDesc>	BlendAttachmentStates;
		float32								BlendConstants[4];
		ELogicOp							LogicOp					= ELogicOp::LOGIC_OP_NO_OP;
		bool								AlphaToCoverageEnable	= true;
		bool								AlphaToOneEnable		= false;
		bool								LogicOpEnable			= false;
	};

	struct InputElementDesc
	{
		String				Semantic	= "";
		uint32				Binding		= 0;
		uint32				Stride		= 0;
		EVertexInputRate	InputRate	= EVertexInputRate::VERTEX_INPUT_NONE;
		uint32				Location	= 0;
		uint32				Offset		= 0;
		EFormat				Format		= EFormat::FORMAT_NONE;
	};

	/*
	* Graphics
	*/
	struct GraphicsPipelineStateDesc
	{
		String					DebugName		= "";
		const RenderPass*		pRenderPass		= nullptr;
		const PipelineLayout*	pPipelineLayout	= nullptr;
		
		// Pipeline
		TArray<InputElementDesc>	InputLayout			= { };
		InputAssemblyDesc			InputAssembly		= { };
		DepthStencilStateDesc		DepthStencilState	= { };
		BlendStateDesc				BlendState			= { };
		RasterizerStateDesc			RasterizerState		= { };
		uint32						SampleMask			= 0;
		uint32						SampleCount			= 1;
		uint32						Subpass				= 0;
		
		// Mesh-Shader pipeline
		ShaderModuleDesc MeshShader;
		ShaderModuleDesc TaskShader;
		
		// Vertex-Shader pipeline
		ShaderModuleDesc VertexShader;
		ShaderModuleDesc HullShader;
		ShaderModuleDesc DomainShader;
		ShaderModuleDesc GeometryShader;
		
		// Common
		ShaderModuleDesc PixelShader;
	};

	/*
	* Compute
	*/
	struct ComputePipelineStateDesc
	{
		String					DebugName		= "";
		const PipelineLayout*	pPipelineLayout	= nullptr;
		ShaderModuleDesc		Shader;
	};


	/*
	* Ray-Tracing 
	*/
	struct RayTracingPipelineStateDesc
	{
		String						DebugName			= "";
		const PipelineLayout*		pPipelineLayout		= nullptr;
		uint32						MaxRecursionDepth	= 1;	
		ShaderModuleDesc			RaygenShader;
		TArray<ShaderModuleDesc>	MissShaders;
		TArray<ShaderModuleDesc>	ClosestHitShaders;
	};

	/*
	* Generic
	*/
	class PipelineState : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(PipelineState);
		
		/*
		* Returns the API-specific handle to the underlaying texture-resource
		*	return - Returns a valid handle on success otherwise zero
		*/
		virtual uint64 GetHandle() const = 0;

		virtual EPipelineStateType GetType() const
		{
			return m_Type;
		}

	protected:
		EPipelineStateType	m_Type;
		String				m_DebugName;
	};
}
