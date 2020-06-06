#pragma once
#include "LambdaEngine.h"

#include "Core/Ref.h"

#include "Resources/ResourceManager.h"

#include "Containers/THashTable.h"
#include "Containers/String.h"

#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/RenderPass.h"

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
		Ref<RenderPass>				RenderPass			= nullptr;
		Ref<PipelineLayout>			PipelineLayout		= nullptr;
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
		String 				DebugName		= "";
		Ref<PipelineLayout>	PipelineLayout	= nullptr;
		ManagedShaderModule	Shader;
	};

	/*
	* Ray-Tracing
	*/
	struct ManagedRayTracingPipelineStateDesc
	{
	public:
		RayTracingPipelineStateDesc GetDesc() const noexcept;

	public:
		String				DebugName			= "";
		Ref<PipelineLayout>	PipelineLayout		= nullptr;
		uint32				MaxRecursionDepth	= 1;
		// Shaders
		ManagedShaderModule			RaygenShader;
		TArray<ManagedShaderModule> MissShaders;
		TArray<ManagedShaderModule> ClosestHitShaders;
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

		static void ReloadPipelineStates();

	private:
		static uint64													s_CurrentPipelineIndex;
		static THashTable<uint64, Ref<PipelineState>>					s_PipelineStates;
		static THashTable<uint64, ManagedGraphicsPipelineStateDesc>		s_GraphicsPipelineStateDescriptions;
		static THashTable<uint64, ManagedComputePipelineStateDesc>		s_ComputePipelineStateDescriptions;
		static THashTable<uint64, ManagedRayTracingPipelineStateDesc>	s_RayTracingPipelineStateDescriptions;
	};
}