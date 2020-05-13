#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/IPipelineState.h"
#include "Resources/ResourceManager.h"

#include "Containers/THashTable.h"

namespace LambdaEngine
{
	struct BlendAttachmentState;

	class IPipelineState;
	class IRenderPass;
	class IPipelineLayout;

	struct GraphicsManagedPipelineStateDesc
	{
		const char*					pName						= "";
		const IRenderPass*			pRenderPass					= nullptr;
		const IPipelineLayout*		pPipelineLayout				= nullptr;

		VertexInputBindingDesc		pVertexInputBindings		[MAX_VERTEX_INPUT_ATTACHMENTS];
		uint32						VertexInputBindingCount		= 0;

		BlendAttachmentState		pBlendAttachmentStates		[MAX_COLOR_ATTACHMENTS];
		uint32						BlendAttachmentStateCount	= 0;

		//New Style
		GUID_Lambda					TaskShader					= GUID_NONE;
		GUID_Lambda					MeshShader					= GUID_NONE;

		//Old Style
		GUID_Lambda					VertexShader				= GUID_NONE;
		GUID_Lambda					GeometryShader				= GUID_NONE;
		GUID_Lambda					HullShader					= GUID_NONE;
		GUID_Lambda					DomainShader				= GUID_NONE;

		//Both
		GUID_Lambda					PixelShader					= GUID_NONE;
	};

	struct ComputeManagedPipelineStateDesc
	{
		const char*				pName						= "";
		const IPipelineLayout*	pPipelineLayout				= nullptr;
		GUID_Lambda				Shader						= GUID_NONE;
	};

	struct RayTracingManagedPipelineStateDesc
	{
		const char*				pName						= "";
		const IPipelineLayout*	pPipelineLayout				= nullptr;
		uint32					MaxRecursionDepth			= 1;	

		GUID_Lambda				RaygenShader				= GUID_NONE;
		GUID_Lambda				pMissShaders[MAX_MISS_SHADER_COUNT];
		GUID_Lambda				pClosestHitShaders[MAX_CLOSEST_HIT_SHADER_COUNT];

		uint32					MissShaderCount				= 0;
		uint32					ClosestHitShaderCount		= 0;
	};

	class LAMBDA_API PipelineStateManager
	{
	public:
		DECL_STATIC_CLASS(PipelineStateManager);
		
		static bool Init();
		static bool Release();

		static uint64 CreateGraphicsPipelineState(const GraphicsManagedPipelineStateDesc* pDesc);
		static uint64 CreateComputePipelineState(const ComputeManagedPipelineStateDesc* pDesc);
		static uint64 CreateRayTracingPipelineState(const RayTracingManagedPipelineStateDesc* pDesc);

		static void ReleasePipelineState(uint64 id);

		static IPipelineState* GetPipelineState(uint64 id);

		static void ReloadPipelineStates();

	private:
		static void FillGraphicsPipelineStateDesc(GraphicsPipelineStateDesc* pDstDesc, const GraphicsManagedPipelineStateDesc* pSrcDesc);
		static void FillComputePipelineStateDesc(ComputePipelineStateDesc* pDstDesc, const ComputeManagedPipelineStateDesc* pSrcDesc);
		static void FillRayTracingPipelineStateDesc(RayTracingPipelineStateDesc* pDstDesc, const RayTracingManagedPipelineStateDesc* pSrcDesc);

	private:
		static uint64															s_CurrentPipelineIndex;
		static std::unordered_map<uint64, IPipelineState*>						s_PipelineStates;
		static std::unordered_map<uint64, GraphicsManagedPipelineStateDesc>		s_GraphicsPipelineStateDescriptions;
		static std::unordered_map<uint64, ComputeManagedPipelineStateDesc>		s_ComputePipelineStateDescriptions;
		static std::unordered_map<uint64, RayTracingManagedPipelineStateDesc>	s_RayTracingPipelineStateDescriptions;
	};
}