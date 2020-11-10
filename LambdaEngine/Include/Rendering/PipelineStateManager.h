#pragma once
#include "LambdaEngine.h"

#include "Core/TSharedRef.h"

#include "Resources/ResourceManager.h"

#include "Containers/THashTable.h"
#include "Containers/String.h"

#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/RenderPass.h"

#include "Application/API/Events/DebugEvents.h"

namespace LambdaEngine
{
	struct ManagedShaderModule
	{
	public:
		operator ShaderModuleDesc() const noexcept;

	public:
		GUID_Lambda				ShaderGUID = GUID_NONE;
		TArray<ShaderConstant>	ShaderConstants;
	};

	/*
	* Graphics
	*/
	struct ManagedGraphicsPipelineStateDesc
	{
	public:
		GraphicsPipelineStateDesc GetDesc() const noexcept;

	public:
		String						DebugName			= "";
		TSharedRef<RenderPass>		RenderPass			= nullptr;
		TSharedRef<PipelineLayout>	PipelineLayout		= nullptr;
		// Pipeline
		TArray<InputElementDesc>	InputLayout			= { };
		InputAssemblyDesc			InputAssembly		= { };
		DepthStencilStateDesc		DepthStencilState	= { };
		BlendStateDesc				BlendState			= { };
		RasterizerStateDesc			RasterizerState		= { };
		uint32						SampleMask			= 0;
		uint32						SampleCount			= 1;
		uint32						Subpass				= 0;
		FExtraDynamicStateFlags		ExtraDynamicState	= FExtraDynamicStateFlag::EXTRA_DYNAMIC_STATE_FLAG_NONE;
		// Mesh-Shader pipeline
		ManagedShaderModule MeshShader;
		ManagedShaderModule TaskShader;
		// Vertex-Shader pipeline
		ManagedShaderModule VertexShader;
		ManagedShaderModule HullShader;
		ManagedShaderModule DomainShader;
		ManagedShaderModule GeometryShader;
		// Common
		ManagedShaderModule PixelShader;
	};

	/*
	* Compute
	*/
	struct ManagedComputePipelineStateDesc
	{
	public:
		ComputePipelineStateDesc GetDesc() const noexcept;

	public:
		String 						DebugName		= "";
		TSharedRef<PipelineLayout>	PipelineLayout	= nullptr;
		ManagedShaderModule			Shader;
	};

	/*
	* Ray-Tracing
	*/
	struct HitGroupShaderModules
	{
		ManagedShaderModule ClosestHitShader;
		ManagedShaderModule AnyHitShader;
		ManagedShaderModule IntersectionShader;
	};


	struct ManagedRayTracingPipelineStateDesc
	{
	public:
		RayTracingPipelineStateDesc GetDesc() const noexcept;

	public:
		String						DebugName			= "";
		TSharedRef<PipelineLayout>	PipelineLayout		= nullptr;
		uint32						MaxRecursionDepth	= 1;
		// Shaders
		ManagedShaderModule			RaygenShader;
		TArray<ManagedShaderModule> MissShaders;
		TArray<HitGroupShaderModules> HitGroupShaders;
	};

	class LAMBDA_API PipelineStateManager
	{
	public:
		DECL_STATIC_CLASS(PipelineStateManager);
		
		static bool Init();
		static bool Release();

		static uint64 CreateGraphicsPipelineState(const ManagedGraphicsPipelineStateDesc* pDesc);
		static uint64 CreateComputePipelineState(const ManagedComputePipelineStateDesc* pDesc);
		static uint64 CreateRayTracingPipelineState(const ManagedRayTracingPipelineStateDesc* pDesc);

		static void ReleasePipelineState(uint64 id);

		static PipelineState* GetPipelineState(uint64 id);

	private:
		static bool OnPipelineStateRecompileEvent(const PipelineStateRecompileEvent& event);

	private:
		static uint64													s_CurrentPipelineIndex;
		static THashTable<uint64, TSharedRef<PipelineState>>			s_PipelineStates;
		static THashTable<uint64, ManagedGraphicsPipelineStateDesc>		s_GraphicsPipelineStateDescriptions;
		static THashTable<uint64, ManagedComputePipelineStateDesc>		s_ComputePipelineStateDescriptions;
		static THashTable<uint64, ManagedRayTracingPipelineStateDesc>	s_RayTracingPipelineStateDescriptions;
	};
}