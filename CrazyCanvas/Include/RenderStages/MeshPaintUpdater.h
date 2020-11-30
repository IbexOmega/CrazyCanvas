#pragma once

#include "Rendering/RenderGraphTypes.h"
#include "Rendering/CustomRenderer.h"
#include "Rendering/Core/API/DescriptorCache.h"
#include "Rendering/Core/API/CommandList.h"

#include "Rendering/Core/API/PipelineContext.h"

#include <unordered_set>

namespace LambdaEngine
{
	class MeshPaintUpdater : public CustomRenderer
	{
	private:
		struct SPushConstantData
		{
			uint32 VertexCount;
			uint32 ShouldResetServer;
		};

	public:
		MeshPaintUpdater();
		~MeshPaintUpdater();

		bool Init();

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* const* pAccelerationStructure) override final;
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, const Sampler* const* ppPerImageSamplers, uint32 imageCount, uint32 subImageCount, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;
		virtual void UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)  override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool Sleeping)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; }

		virtual const String& GetName() const override final
		{
			static String name = RENDER_GRAPH_MESH_PAINT_UPDATER_NAME;
			return name;
		}

		static void ClearServer(Entity entity);

	private:
		bool CreatePipelineLayout();
		bool CreateDescriptorSets();
		bool CreateShaders();
		bool CreateCommandLists();

	private:
		bool								m_Initilized = false;

		TSharedRef<Sampler>					m_Sampler = nullptr;
		TSharedRef<DescriptorHeap>			m_DescriptorHeap = nullptr;
		PipelineContext						m_UpdatePipeline;

		TArray<uint32>						m_VertexCountList;
		TArray<std::pair<Entity, TSharedRef<DescriptorSet>>>	m_DrawArgDescriptorSets;

		uint32								m_BackBufferCount = 0;
		uint32								m_CurrentFrameIndex = 0;

		CommandAllocator** m_ppComputeCommandAllocators = nullptr;
		CommandList** m_ppComputeCommandLists = nullptr;

		TArray<TArray<DeviceChild*>> m_ResourcesToRemove;

		static std::unordered_set<Entity> s_EntitiesToClear;
	};
}

