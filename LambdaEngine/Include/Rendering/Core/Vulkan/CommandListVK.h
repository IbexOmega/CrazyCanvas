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

	class CommandListVK : public TDeviceChildBase<GraphicsDeviceVK, CommandList>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, CommandList>;

	public:
		CommandListVK(const GraphicsDeviceVK* pDevice);
		~CommandListVK();

		bool Init(CommandAllocator* pAllocator, const CommandListDesc* pDesc);

		FORCEINLINE VkCommandBuffer GetCommandBuffer() const
		{
			return m_CommandList;
		}

	public:
		// DeviceChild interface
		virtual void SetName(const String& name) override;

		// CommandList interface
		virtual bool Begin(const SecondaryCommandListBeginDesc* pBeginDesc)	override final;
		virtual bool End()													override final;

		virtual void BeginRenderPass(const BeginRenderPassDesc* pBeginDesc) override final;
		virtual void EndRenderPass() override final;

		virtual void BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* pBuildDesc)		override final;
		virtual void BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* pBuildDesc)	override final;

		virtual void CopyBuffer(const Buffer* pSrc, uint64 srcOffset, Buffer* pDst, uint64 dstOffset, uint64 sizeInBytes)				override final;
		virtual void CopyTextureFromBuffer(const Buffer* pSrc, Texture* pDst, const CopyTextureFromBufferDesc& desc)					override final;
		
		virtual void BlitTexture(const Texture* pSrc, ETextureState srcState, Texture* pDst, ETextureState dstState, EFilterType filter)	override final;

		virtual void PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrierDesc* pTextureBarriers, uint32 textureBarrierCount)	override final;
		virtual void PipelineBufferBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineBufferBarrierDesc* pBufferBarriers, uint32 bufferBarrierCount)		override final;
		virtual void PipelineMemoryBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineMemoryBarrierDesc* pMemoryBarriers, uint32 bufferMemoryCount)			override final;

		virtual void GenerateMiplevels(Texture* pTexture, ETextureState stateBefore, ETextureState stateAfter) override final;

		virtual void SetViewports(const Viewport* pViewports, uint32 firstViewport, uint32 viewportCount)           override final;
		virtual void SetScissorRects(const ScissorRect* pScissorRects, uint32 firstScissor, uint32 scissorCount)    override final;

		virtual void SetConstantRange(const PipelineLayout* pPipelineLayout, uint32 shaderStageMask, const void* pConstants, uint32 size, uint32 offset) override final;

		virtual void BindIndexBuffer(const Buffer* pIndexBuffer, uint64 offset, EIndexType indexType) override;
		virtual void BindVertexBuffers(const Buffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount) override final;

		virtual void BindDescriptorSetGraphics(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex) override final;
		virtual void BindDescriptorSetCompute(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex) override final;
		virtual void BindDescriptorSetRayTracing(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex) override final;

		virtual void BindGraphicsPipeline(const PipelineState* pPipeline)	override final;
		virtual void BindComputePipeline(const PipelineState* pPipeline)	override final;
		virtual void BindRayTracingPipeline(PipelineState* pPipeline)		override final;

		virtual void TraceRays(uint32 width, uint32 height, uint32 depth) override final;

		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ) override final;

		virtual void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)                          override final;
		virtual void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)  override final;
		virtual void DrawIndexedIndirect(const Buffer* pDrawBuffer, uint32 offset, uint32 drawCount, uint32 stride)							override final;

		virtual void BeginQuery(QueryHeap* pQueryHeap, uint32 queryIndex)										override final;
		virtual void Timestamp(QueryHeap* pQueryHeap, uint32 queryIndex, FPipelineStageFlags pipelineStageFlag)	override final;
		virtual void EndQuery(QueryHeap* pQueryHeap, uint32 queryIndex)											override final;

		virtual void ExecuteSecondary(const CommandList* pSecondary) override final;

		virtual CommandAllocator* GetAllocator() override final;

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return reinterpret_cast<uint64>(m_CommandList);
		}
	
	private:
		void BindDescriptorSet(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex, VkPipelineBindPoint bindPoint);

	private:
		VkCommandBuffer							m_CommandList				= VK_NULL_HANDLE;
		TSharedRef<CommandAllocatorVK>			m_Allocator					= nullptr;
		TSharedRef<RayTracingPipelineStateVK>	m_CurrentRayTracingPipeline	= nullptr;
		
		VkMemoryBarrier			m_MemoryBarriers[MAX_MEMORY_BARRIERS];
		VkImageMemoryBarrier	m_ImageBarriers[MAX_IMAGE_BARRIERS];
		VkBufferMemoryBarrier	m_BufferBarriers[MAX_BUFFER_BARRIERS];
		VkViewport				m_Viewports[MAX_VIEWPORTS];
		VkRect2D				m_ScissorRects[MAX_VIEWPORTS];
		VkBuffer				m_VertexBuffers[MAX_VERTEX_BUFFERS];
		VkDeviceSize			m_VertexBufferOffsets[MAX_VERTEX_BUFFERS];
		VkClearValue			m_ClearValues[MAX_COLOR_ATTACHMENTS+1];
	};
}
