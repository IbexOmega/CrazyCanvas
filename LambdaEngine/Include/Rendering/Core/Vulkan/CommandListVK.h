#pragma once
#include "Core/TSharedRef.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class CommandAllocatorVK;
	class RayTracingPipelineStateVK;

	/*
	* DeferredImageBarrier
	*/
	struct DeferredImageBarrier
	{
	public:
		inline bool HasCompatableStages(const DeferredImageBarrier& other) const
		{
			return (SrcStages == other.SrcStages) && (DestStages == other.DestStages);
		}

	public:
		VkPipelineStageFlags SrcStages	= 0;
		VkPipelineStageFlags DestStages	= 0;
		TArray<VkImageMemoryBarrier> Barriers;
	};

	/*
	* CommandListVK
	*/
	class CommandListVK : public TDeviceChildBase<GraphicsDeviceVK, CommandList>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, CommandList>;

	public:
		CommandListVK(const GraphicsDeviceVK* pDevice);
		~CommandListVK();

		bool Init(CommandAllocator* pAllocator, const CommandListDesc* pDesc);

		FORCEINLINE VkCommandBuffer GetCommandBuffer() const
		{
			return m_CmdBuffer;
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override;

		// CommandList interface
		virtual bool Begin(const SecondaryCommandListBeginDesc* pBeginDesc) override final;
		virtual bool End() override final;

		virtual void BeginRenderPass(const BeginRenderPassDesc* pBeginDesc) override final;
		virtual void EndRenderPass() override final;

		virtual void BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* pBuildDesc) override final;
		virtual void BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* pBuildDesc) override final;

		virtual void CopyBuffer(
			const Buffer* pSrc,
			uint64 srcOffset,
			Buffer* pDst,
			uint64 dstOffset,
			uint64 sizeInBytes) override final;

		virtual void CopyTextureFromBuffer(
			const Buffer* pSrc,
			Texture* pDst,
			const CopyTextureBufferDesc& desc) override final;

		virtual void CopyTextureToBuffer(
			const Texture* pSrc,
			Buffer* pDst,
			const CopyTextureBufferDesc& desc) override final;

		virtual void BlitTexture(
			const Texture* pSrc,
			ETextureState srcState,
			Texture* pDst,
			ETextureState dstState,
			EFilterType filter) override final;

		virtual void TransitionBarrier(
			Texture* pResource,
			FPipelineStageFlags srcStage,
			FPipelineStageFlags dstStage,
			uint32 srcAccessMask,
			uint32 destAccessMask,
			ETextureState beforeState,
			ETextureState afterState) override final;

		virtual void TransitionBarrier(
			Texture* pResource,
			FPipelineStageFlags srcStage,
			FPipelineStageFlags dstStage,
			uint32 srcAccessMask,
			uint32 destAccessMask,
			uint32 arrayIndex,
			uint32 arrayCount,
			ETextureState beforeState,
			ETextureState afterState) override final;
      
		virtual void QueueTransferBarrier(
			Texture* pResource,
			FPipelineStageFlags srcStage,
			FPipelineStageFlags dstStage,
			uint32 srcAccessMask,
			uint32 destAccessMask,
			ECommandQueueType srcQueue,
			ECommandQueueType dstQueue,
			ETextureState beforeState,
			ETextureState afterState) override final;

		virtual void PipelineTextureBarriers(
			FPipelineStageFlags srcStage,
			FPipelineStageFlags dstStage,
			const PipelineTextureBarrierDesc* pTextureBarriers,
			uint32 textureBarrierCount) override final;

		virtual void PipelineBufferBarriers(
			FPipelineStageFlags srcStage,
			FPipelineStageFlags dstStage,
			const PipelineBufferBarrierDesc* pBufferBarriers,
			uint32 bufferBarrierCount) override final;

		virtual void PipelineMemoryBarriers(
			FPipelineStageFlags srcStage,
			FPipelineStageFlags dstStage,
			const PipelineMemoryBarrierDesc* pMemoryBarriers,
			uint32 bufferMemoryCount) override final;

		virtual void GenerateMips(Texture* pTexture, ETextureState stateBefore, ETextureState stateAfter, bool linearFiltering) override final;

		virtual void SetViewports(const Viewport* pViewports, uint32 firstViewport, uint32 viewportCount) override final;
		virtual void SetScissorRects(const ScissorRect* pScissorRects, uint32 firstScissor, uint32 scissorCount) override final;
		virtual void SetStencilTestReference(EStencilFace face, uint32 reference) override final;

		virtual void SetConstantRange(
			const PipelineLayout* pPipelineLayout,
			uint32 shaderStageMask,
			const void* pConstants,
			uint32 size,
			uint32 offset) override final;

		virtual void BindIndexBuffer(const Buffer* pIndexBuffer, uint64 offset, EIndexType indexType) override final;
		virtual void BindVertexBuffers(
			const Buffer* const* ppVertexBuffers,
			uint32 firstBuffer,
			const uint64* pOffsets,
			uint32 vertexBufferCount) override final;

		virtual void BindDescriptorSetGraphics(
			const DescriptorSet* pDescriptorSet,
			const PipelineLayout* pPipelineLayout,
			uint32 setIndex) override final;

		virtual void BindDescriptorSetCompute(
			const DescriptorSet* pDescriptorSet,
			const PipelineLayout* pPipelineLayout,
			uint32 setIndex) override final;

		virtual void BindDescriptorSetRayTracing(
			const DescriptorSet* pDescriptorSet,
			const PipelineLayout* pPipelineLayout,
			uint32 setIndex) override final;

		virtual void BindGraphicsPipeline(const PipelineState* pPipeline) override final;
		virtual void BindComputePipeline(const PipelineState* pPipeline) override final;
		virtual void BindRayTracingPipeline(PipelineState* pPipeline) override final;

		virtual void TraceRays(const SBT* pSBT, uint32 width, uint32 height, uint32 depth) override final;

		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ) override final;

		virtual void DispatchMesh(uint32 taskCount, uint32 firstTask) override final;
		virtual void DispatchMeshIndirect(
			const Buffer* pDrawBuffer,
			uint32 offset,
			uint32 drawCount,
			uint32 stride) override final;

		virtual void DrawInstanced(
			uint32 vertexCount,
			uint32 instanceCount,
			uint32 firstVertex,
			uint32 firstInstance) override final;

		virtual void DrawIndexInstanced(
			uint32 indexCount,
			uint32 instanceCount,
			uint32 firstIndex,
			uint32 vertexOffset,
			uint32 firstInstance) override final;

		virtual void DrawIndexedIndirect(
			const Buffer* pDrawBuffer,
			uint32 offset,
			uint32 drawCount,
			uint32 stride) override final;

		virtual void BeginQuery(QueryHeap* pQueryHeap, uint32 queryIndex) override final;

		virtual void Timestamp(
			QueryHeap* pQueryHeap,
			uint32 queryIndex,
			FPipelineStageFlags pipelineStageFlag) override final;

		virtual void EndQuery(QueryHeap* pQueryHeap, uint32 queryIndex) override final;
		virtual void ResetQuery(QueryHeap* pQueryHeap, uint32 firstQuery, uint32 queryCount) override final;

		virtual void SetLineWidth(float32 lineWidth) override final;

		virtual void DeferDestruction(DeviceChild* pResource) override final;

		virtual void ExecuteSecondary(const CommandList* pSecondary) override final;

		virtual void FlushDeferredBarriers() override final;
		virtual void FlushDeferredResources() override final;

		virtual CommandAllocator* GetAllocator() override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_CmdBuffer);
		}

	private:
		void BindDescriptorSet(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex, VkPipelineBindPoint bindPoint);

	private:
		VkCommandBuffer							m_CmdBuffer				= VK_NULL_HANDLE;
		TSharedRef<CommandAllocatorVK>			m_Allocator					= nullptr;
		TSharedRef<RayTracingPipelineStateVK>	m_CurrentRayTracingPipeline	= nullptr;

		TArray<DeferredImageBarrier> m_DeferredBarriers;
		TArray<TSharedRef<DeviceChild>> m_ResourcesToDestroy;

		VkMemoryBarrier			m_MemoryBarriers[MAX_MEMORY_BARRIERS];
		VkImageMemoryBarrier	m_ImageBarriers[MAX_IMAGE_BARRIERS];
		VkBufferMemoryBarrier	m_BufferBarriers[MAX_BUFFER_BARRIERS];
		VkViewport				m_Viewports[MAX_VIEWPORTS];
		VkRect2D				m_ScissorRects[MAX_VIEWPORTS];
		VkBuffer				m_VertexBuffers[MAX_VERTEX_BUFFERS];
		VkDeviceSize			m_VertexBufferOffsets[MAX_VERTEX_BUFFERS];
		VkClearValue			m_ClearValues[MAX_COLOR_ATTACHMENTS + 1];
	};
}
