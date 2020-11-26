#pragma once

#include "Containers/String.h"
#include "Rendering/Core/API/PipelineContext.h"

#include "Rendering/CustomRenderer.h"

namespace LambdaEngine
{
	class Buffer;
	class CommandList;
	class CommandAllocator;
	class DescriptorHeap;
	class DescriptorSet;

	struct DrawArg;
}

class HealthCompute : public LambdaEngine::CustomRenderer
{
public:
	HealthCompute();
	~HealthCompute();

	virtual bool Init() override final;

	virtual bool RenderGraphInit(const LambdaEngine::CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

	virtual void Update(
		LambdaEngine::Timestamp delta,
		uint32 modFrameIndex,
		uint32 backBufferIndex) override final;

	virtual void UpdateBufferResource(
		const LambdaEngine::String& resourceName,
		const LambdaEngine::Buffer* const* ppBuffers,
		uint64* pOffsets,
		uint64* pSizesInBytes,
		uint32 count,
		bool backBufferBound) override final;

	virtual void UpdateDrawArgsResource(
		const LambdaEngine::String& resourceName,
		const LambdaEngine::DrawArg* pDrawArgs,
		uint32 count) override final;

	virtual void Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		LambdaEngine::CommandList** ppFirstExecutionStage,
		LambdaEngine::CommandList** ppSecondaryExecutionStage,
		bool sleeping) override final;

	virtual LambdaEngine::FPipelineStageFlag GetFirstPipelineStage() const override final
	{
		return LambdaEngine::FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
	};

	virtual LambdaEngine::FPipelineStageFlag GetLastPipelineStage() const override final
	{
		return LambdaEngine::FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER;
	};

	virtual const LambdaEngine::String& GetName() const override final
	{
		static LambdaEngine::String name = "COMPUTE_HEALTH";
		return name;
	};

public:
	static const LambdaEngine::TArray<uint32>& GetHealths();
	static void QueueHealthCalculation(LambdaEngine::Entity entity);
	static uint32 GetVertexCount();
	static uint32 GetEntityHealth(LambdaEngine::Entity entity);

private:
	bool CreatePipelineLayout();
	bool CreateDescriptorSets();
	bool CreateShaders();
	bool CreateCommandLists();
	bool CreateResources();
	void ResetHealthBuffer(LambdaEngine::CommandList* pCommandList);

private:
	bool m_Initilized			= false;
	bool m_ResetHealthBuffer	= true;
	uint32 m_BackBufferCount	= 0;

	LambdaEngine::PipelineContext m_PipelineContext;

	LambdaEngine::CommandAllocator**	m_ppComputeCommandAllocators		= nullptr;
	LambdaEngine::CommandList**			m_ppComputeCommandLists				= nullptr;
	LambdaEngine::TSharedRef<LambdaEngine::DescriptorHeap> m_DescriptorHeap	= nullptr;

	LambdaEngine::Buffer*	m_pHealthBuffer			= nullptr;
	LambdaEngine::Buffer*	m_pCopyBuffer			= nullptr;
	LambdaEngine::Buffer*	m_pVertexCountBuffer	= nullptr;

	LambdaEngine::TArray<LambdaEngine::DescriptorSet*> m_DrawArgsDescriptorSets;


private:
	static LambdaEngine::TArray<uint32> s_Healths;
	static LambdaEngine::TSet<LambdaEngine::Entity> s_HealthsToCalculate;
	static uint32 s_VertexCount;
};